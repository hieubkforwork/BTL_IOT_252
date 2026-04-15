import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
import tensorflow as tf
import numpy as np
import joblib

PREFIX = "TEST_5_TinyML"

data = pd.read_csv("data.csv", names=["temp", "humi", "label"])

X = data[["temp", "humi"]].values
y = data["label"].values

scaler = StandardScaler()
X = scaler.fit_transform(X)

print("\n" + "="*50)
print("SCALER VALUES FOR ESP32")
print("="*50)
print(f"Mean: {scaler.mean_}")
print(f"Scale: {scaler.scale_}")
print("Formula: (value - mean) / scale")
print("="*50 + "\n")

X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.2, random_state=42
)

model = tf.keras.Sequential([
    tf.keras.layers.Input(shape=(2,)),
    tf.keras.layers.Dense(16, activation='relu'),
    tf.keras.layers.Dense(8, activation='relu'),
    tf.keras.layers.Dense(1, activation='sigmoid')
])

model.compile(
    loss="binary_crossentropy",
    optimizer=tf.keras.optimizers.Adam(learning_rate=0.001),
    metrics=["accuracy"]
)

# Early Stopping - Train tự dừng khi đủ
early_stop = tf.keras.callbacks.EarlyStopping(
    monitor='val_loss',
    patience=10,
    restore_best_weights=True,
    verbose=1
)

model.fit(
    X_train,
    y_train,
    epochs=100,
    batch_size=8,
    validation_data=(X_test, y_test),
    callbacks=[early_stop]
)

print("\n" + "="*50)
print("EVALUATING MODEL ON TEST DATA")
print("="*50)
loss, acc = model.evaluate(X_test, y_test, verbose=0)
print(f"Test Loss: {loss:.4f}")
print(f"Test Accuracy: {acc*100:.2f}%")
print("="*50 + "\n")

model.save(PREFIX + '.h5')

joblib.dump(scaler, "scaler.save")
print("Scaler saved: scaler.save")
print("  (ESP32 cần file này để scale sensor input trước inference)\n")

print("Converting to TFLite with INT8 quantization...")

# Representative dataset cho quantization
def representative_dataset():
    for i in range(min(100, len(X_train))):
        yield [X_train[i:i+1].astype(np.float32)]

converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.representative_dataset = representative_dataset
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.int8
converter.inference_output_type = tf.int8
tflite_model = converter.convert()
print("INT8 quantization complete")

with open(PREFIX + ".tflite", "wb") as f:
    f.write(tflite_model)

print(f"Model size: {len(tflite_model)} bytes (INT8 compressed)\n")

tflite_path = PREFIX + '.tflite'
output_header_path = PREFIX + '.h'

with open(tflite_path, 'rb') as tflite_file:
    tflite_content = tflite_file.read()

hex_lines = [', '.join([f'0x{byte:02x}' for byte in tflite_content[i:i + 12]]) for i in
         range(0, len(tflite_content), 12)]

hex_array = ',\n  '.join(hex_lines)

with open(output_header_path, 'w') as header_file:
    header_file.write('const unsigned char model[] = {\n  ')
    header_file.write(f'{hex_array}\n')
    header_file.write('};\n\n')
