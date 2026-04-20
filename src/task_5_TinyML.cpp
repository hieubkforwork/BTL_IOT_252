#include "task_5_TinyML.h"
#include "task_2_ledNeoPixel.h"
#include "task_6_CoreIOT.h"
namespace
{
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;
    constexpr int kTensorArenaSize = 8 * 1024; 
    uint8_t tensor_arena[kTensorArenaSize];
    
    // Performance metrics tracking
    struct {
        unsigned long inferenceCount = 0;
        unsigned long minTime = 99999;
        unsigned long maxTime = 0;
        unsigned long totalTime = 0;
        float avgTime = 0.0f;
    } perf;
} // namespace

void setupTinyML()
{
    Serial.println("TensorFlow Lite Init....");
    
    // Create error reporter for TensorFlow Lite logging
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    // Load model: embedded constant array from header file (772 bytes INT8 quantized)
    model = tflite::GetModel(dht_anomaly_model_tflite); 
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        // Version mismatch indicates incompatible model format
        error_reporter->Report("Model provided is schema version %d, not equal to supported version %d.",
                               model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    // Create operation resolver: register all supported ops (ReLU, Dense, Sigmoid, etc.)
    static tflite::AllOpsResolver resolver;
    
    // Create interpreter: instantiates the inference engine with custom tensor arena
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    // Allocate tensors: prepare buffers for all intermediate activations
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        error_reporter->Report("AllocateTensors() failed");
        return;
    }

    // Cache tensor pointers for zero-copy access during inference
    input = interpreter->input(0);   // Input tensor: [temp_norm, humidity_norm]
    output = interpreter->output(0); // Output tensor: [confidence_score]

    Serial.println("TensorFlow Lite Micro initialized on ESP32.");
}

void tiny_ml_task(void *pvParameters)
{
    // One-time setup: initialize TensorFlow Lite runtime
    setupTinyML();
    
    // Scaler constants from training (critical for accuracy!)
    // Derived from: StandardScaler().fit(training_data)
    const float TEMP_MEAN = 22.71924051f;   // Mean temperature from 395 training samples
    const float TEMP_SCALE = 11.55387094f;  // Std dev of temperature
    const float HUMI_MEAN = 51.16151899f;   // Mean humidity from 395 training samples
    const float HUMI_SCALE = 23.591529f;    // Std dev of humidity

    while (1)
    {
        float temp_normalized = (glob_temperature - TEMP_MEAN) / TEMP_SCALE;
        float humi_normalized = (glob_humidity - HUMI_MEAN) / HUMI_SCALE;

        // ========== STEP 3: Write to input tensor ==========
        // Input tensor layout: [0]=temperature, [1]=humidity
        input->data.f[0] = temp_normalized;
        input->data.f[1] = humi_normalized;

        // ========== STEP 4: Run inference (neural network forward pass) ==========
        // Forward pass: Input → Dense(16,relu) → Dense(8,relu) → Dense(1,sigmoid) → Output
        unsigned long start_time = millis();
        TfLiteStatus invoke_status = interpreter->Invoke();  // Execute network
        unsigned long inference_time = millis() - start_time; // Measure execution time (ms)

        // Check for execution errors
        if (invoke_status != kTfLiteOk)
        {
            error_reporter->Report("Invoke failed");
            return;  // Critical error: stop task
        }

        // ========== STEP 5: Read output confidence score ==========
        // Output range: [0.0, 1.0] (sigmoid activation)
        // 0.0-0.5 = NORMAL, 0.5-1.0 = ANOMALY (threshold=0.5)
        float result = output->data.f[0];
        
        // ========== STEP 6: Update performance statistics ==========
        // Track inference performance on real hardware
        perf.inferenceCount++;                       // Increment run count
        perf.totalTime += inference_time;            // Accumulate time
        if (inference_time < perf.minTime) {
            perf.minTime = inference_time;           // Track fastest execution
        }
        if (inference_time > perf.maxTime) {
            perf.maxTime = inference_time;           // Track slowest execution
        }
        perf.avgTime = (float)perf.totalTime / perf.inferenceCount;  // Running average

        // ========== STEP 7: Output results ==========
        // Print sensor inputs, inference output, inference time, performance stats to Serial
        Serial.printf("Temp: %.1f°C | Humi: %.1f%%\n", glob_temperature, glob_humidity);
        Serial.printf("Inference Time: %ld ms\n", inference_time);
        Serial.printf("Confidence: %.3f\n", result);
        Serial.printf("Status: %s\n", result > 0.5 ? "ANOMALY" : "NORMAL");
        Serial.printf("Stats - Min: %lu ms | Max: %lu ms | Avg: %.1f ms (n=%lu)\n", 
                     perf.minTime,
                     perf.maxTime,
                     perf.avgTime,
                     perf.inferenceCount);
        Serial.printf("Free RAM: %u bytes\n", xPortGetFreeHeapSize());
        
        // ========== STEP 8: Publish to cloud (MQTT via Task 6 CoreIOT) ==========
        // Relay TinyML results to MQTT broker → CoreIOT cloud dashboard
        const char* statusStr = (result > 0.5) ? "ANOMALY" : "NORMAL";
        publishTinyMLData(glob_temperature, glob_humidity, result, 
                         statusStr, inference_time, xPortGetFreeHeapSize());
    
        // ========== STEP 9: Real-time feedback via NeoPixel LED ==========
        // Visual indication of anomaly status
        if (result > 0.5) {
            updateNeoPixel(0xFF0000, 0, 0);  // RED: ANOMALY detected (constantly on)
        } else {
            updateNeoPixel(0x00FF00, 0, 0);  // GREEN: NORMAL condition (constantly on)
        }

        // ========== STEP 10: Sleep and repeat ==========
        // Run inference every 5 seconds (non-blocking FreeRTOS delay)
        vTaskDelay(5000);
    }
}
