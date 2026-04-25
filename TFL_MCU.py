import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
import tensorflow as tf
import numpy as np
import joblib

PREFIX = "TEST_5_TinyML"

data = pd.read_csv("data.csv", names=["temp", "humi", "label"])
data["label"] = data["label"].astype(int)

X = data[["temp", "humi"]].values
y = data["label"].values

# Fit scaler on full dataset
scaler = StandardScaler()
X_scaled = scaler.fit_transform(X)

print("\n" + "="*60)
print("SCALER PARAMETERS (for ESP32)")
print("="*60)
print(f"Temperature - Mean: {scaler.mean_[0]:.8f}, Std: {scaler.scale_[0]:.8f}")
print(f"Humidity    - Mean: {scaler.mean_[1]:.8f}, Std: {scaler.scale_[1]:.8f}")
print("Formula: (x - mean) / std")
print("="*60 + "\n")

# Save scaler for ESP32 reference
joblib.dump(scaler, "scaler.save")
print("[+] Scaler saved to scaler.save\n")

# Split data (use SCALED data for training)
X_train, X_test, y_train, y_test = train_test_split(
    X_scaled, y, test_size=0.2, random_state=42
)

# Build model (NO Lambda layer - simpler conversion)
model = tf.keras.Sequential([
    tf.keras.layers.Input(shape=(2,), name="normalized_input"),
    tf.keras.layers.Dense(16, activation='relu', name="dense_16"),
    tf.keras.layers.Dense(8, activation='relu', name="dense_8"),
    tf.keras.layers.Dense(1, activation='sigmoid', name="output_sigmoid")
])

model.compile(
    loss="binary_crossentropy",
    optimizer=tf.keras.optimizers.Adam(learning_rate=0.001),
    metrics=["accuracy"]
)

print("Model Architecture:")
model.summary()
print()

# Early Stopping
early_stop = tf.keras.callbacks.EarlyStopping(
    monitor='val_loss',
    patience=10,
    restore_best_weights=True,
    verbose=1
)

print("Training model on normalized data...")
model.fit(
    X_train,
    y_train,
    epochs=100,
    batch_size=8,
    validation_data=(X_test, y_test),
    callbacks=[early_stop],
    class_weight=None
)

print("\n" + "="*60)
print("EVALUATING MODEL ON TEST DATA")
print("="*60)
loss, acc = model.evaluate(X_test, y_test, verbose=0)
print(f"Test Loss: {loss:.4f}")
print(f"Test Accuracy: {acc*100:.2f}%")
print("="*60 + "\n")

# Save Keras model
model.save(PREFIX + '.h5')
print(f"[+] Keras model saved: {PREFIX}.h5\n")

# Convert to TFLite FLOAT32 (NO Lambda layer issue)
print("Converting to TFLite FLOAT32 (no embedded normalization)...")
print("  ESP32 will: normalize raw input THEN feed to model\n")

converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = []  # No quantization
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS]
converter.inference_input_type = tf.float32
converter.inference_output_type = tf.float32
tflite_model = converter.convert()

with open(PREFIX + ".tflite", "wb") as f:
    f.write(tflite_model)

print(f"[+] Model saved: {PREFIX}.tflite ({len(tflite_model)} bytes)\n")

# Generate C++ header
tflite_path = PREFIX + '.tflite'
output_header_path = "include/dht_anomaly_model.h"

with open(tflite_path, 'rb') as tflite_file:
    tflite_content = tflite_file.read()

hex_lines = [', '.join([f'0x{byte:02x}' for byte in tflite_content[i:i + 12]]) 
             for i in range(0, len(tflite_content), 12)]
hex_array = ',\n  '.join(hex_lines)

with open(output_header_path, 'w') as header_file:
    header_file.write('// TFLite model (expects normalized input)\n')
    header_file.write(f'// Size: {len(tflite_content)} bytes\n\n')
    header_file.write('const unsigned char model[] = {\n  ')
    header_file.write(f'{hex_array}\n')
    header_file.write('};\n')

print(f"[+] C++ header generated: {output_header_path}\n")
print("="*60)
print("DEPLOYMENT READY")
print("="*60)
print("[+] Scaler saved: scaler.save")
print("[+] Model expects NORMALIZED inputs")
print("[+] ESP32: normalize (x - mean) / std, then invoke model")
print("="*60)
