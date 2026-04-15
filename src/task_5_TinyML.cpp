#include "task_5_TinyML.h"
#include "task_2_ledNeoPixel.h"

// Globals, for the convenience of one-shot setup.
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
        unsigned long total_inferences = 0;
        unsigned long min_inference_time = 99999;
        unsigned long max_inference_time = 0;
        unsigned long total_inference_time = 0;
        float avg_inference_time = 0.0f;
    } performance_stats;
} // namespace

void setupTinyML()
{
    Serial.println("TensorFlow Lite Init....");
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(dht_anomaly_model_tflite); 
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        error_reporter->Report("Model provided is schema version %d, not equal to supported version %d.",
                               model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        error_reporter->Report("AllocateTensors() failed");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

    Serial.println("TensorFlow Lite Micro initialized on ESP32.");
}

void tiny_ml_task(void *pvParameters)
{

    setupTinyML();
    const float TEMP_MEAN = 22.71924051f;
    const float TEMP_SCALE = 11.55387094f;
    const float HUMI_MEAN = 51.16151899f;
    const float HUMI_SCALE = 23.591529f;

    while (1)
    {
        float temp_normalized = (glob_temperature - TEMP_MEAN) / TEMP_SCALE;
        float humi_normalized = (glob_humidity - HUMI_MEAN) / HUMI_SCALE;

        input->data.f[0] = temp_normalized;
        input->data.f[1] = humi_normalized;

        // Measure inference time
        unsigned long start_time = millis();
        TfLiteStatus invoke_status = interpreter->Invoke();
        unsigned long inference_time = millis() - start_time;

        if (invoke_status != kTfLiteOk)
        {
            error_reporter->Report("Invoke failed");
            return;
        }

        // Get and process output
        float result = output->data.f[0];
        
        // Update performance statistics
        performance_stats.total_inferences++;
        performance_stats.total_inference_time += inference_time;
        if (inference_time < performance_stats.min_inference_time) {
            performance_stats.min_inference_time = inference_time;
        }
        if (inference_time > performance_stats.max_inference_time) {
            performance_stats.max_inference_time = inference_time;
        }
        performance_stats.avg_inference_time = (float)performance_stats.total_inference_time / performance_stats.total_inferences;

        Serial.printf("Temp: %.1f°C | Humi: %.1f%%\n", glob_temperature, glob_humidity);
        Serial.printf("Inference Time: %ld ms\n", inference_time);
        Serial.printf("Confidence: %.3f\n", result);
        Serial.printf("Status: %s\n", result > 0.5 ? "ANOMALY" : "NORMAL");
        Serial.printf("Stats - Min: %lu ms | Max: %lu ms | Avg: %.1f ms (n=%lu)\n", 
                     performance_stats.min_inference_time,
                     performance_stats.max_inference_time,
                     performance_stats.avg_inference_time,
                     performance_stats.total_inferences);
        Serial.printf("Free RAM: %u bytes\n", xPortGetFreeHeapSize());
        
    
        if (result > 0.5) {
            updateNeoPixel(0xFF0000, 0, 0);  // Red: constantly on
        } else {
            updateNeoPixel(0x00FF00, 0, 0);  // Green: constantly on
        }

        vTaskDelay(5000);
    }
}
