#include "task_5_TinyML.h"
#include "task_2_ledNeoPixel.h"
#include "task_6_CoreIOT.h"
namespace
{
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *tflite_model = nullptr; 
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;
    constexpr int kTensorArenaSize = 32 * 1024;  
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
    
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    tflite_model = tflite::GetModel(model);  // 'model' is the byte array from dht_anomaly_model.h
    if (tflite_model->version() != TFLITE_SCHEMA_VERSION)
    {
        error_reporter->Report("Model provided is schema version %d, not equal to supported version %d.",
                               tflite_model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    static tflite::AllOpsResolver resolver;
    
    static tflite::MicroInterpreter static_interpreter(
        tflite_model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        Serial.printf("[SETUP] ERROR: AllocateTensors() failed! Status=%d\n", allocate_status);
        error_reporter->Report("AllocateTensors() failed");
        return;
    }
    Serial.println("[SETUP] AllocateTensors() SUCCESS");

    input = interpreter->input(0); 
    output = interpreter->output(0); 
    
    if (input == nullptr) {
        Serial.println("[SETUP] ERROR: Input tensor is NULL!");
        return;
    }
    if (output == nullptr) {
        Serial.println("[ERROR] Output tensor is NULL!");
        return;
    }
}

void tiny_ml_task(void *pvParameters)
{

    setupTinyML();
    
    const float TEMP_MEAN = 25.56537500f;   
    const float TEMP_STD = 12.23954916f;  
    const float HUMI_MEAN = 53.83450000f;   
    const float HUMI_STD = 21.54992366f;
    const TickType_t xInferencePeriod = pdMS_TO_TICKS(1000);
    static bool lastAnomalyState = false;

    while (1)
    {
        float temp_raw = glob_temperature;
        float humi_raw = glob_humidity;

        float temp_norm = (temp_raw - TEMP_MEAN) / TEMP_STD;
        float humi_norm = (humi_raw - HUMI_MEAN) / HUMI_STD;

        if (interpreter == nullptr || input == nullptr || output == nullptr) {
            Serial.println("[ERROR] Interpreter or tensors NULL!");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        
        input->data.f[0] = temp_norm;
        input->data.f[1] = humi_norm;

        unsigned long start_time = millis();
        TfLiteStatus invoke_status = interpreter->Invoke();
        unsigned long inference_time = millis() - start_time;

        if (invoke_status != kTfLiteOk) {
            Serial.printf("[INVOKE] FAILED! Status: %d\n", invoke_status);
            vTaskDelay(xInferencePeriod);
            continue;
        }

        float result = output->data.f[0];
        
        perf.inferenceCount++;
        perf.totalTime += inference_time;
        if (inference_time < perf.minTime) perf.minTime = inference_time;
        if (inference_time > perf.maxTime) perf.maxTime = inference_time;
        perf.avgTime = (float)perf.totalTime / perf.inferenceCount;

        const bool isAnomaly = (result > 0.5f);
        const char* status = isAnomaly ? "ANOMALY" : "NORMAL";

        if (isAnomaly != lastAnomalyState || (perf.inferenceCount % 5) == 0) {
            Serial.printf("[Task5] infer=%u time=%lums avg=%.2fms status=%s\n",
                          (unsigned)perf.inferenceCount,
                          inference_time,
                          perf.avgTime,
                          status);
            lastAnomalyState = isAnomaly;
        }

        if (isAnomaly) {
            updateNeoPixel(0xFF0000, 0, 0);
        } else {
            updateNeoPixel(0x00FF00, 0, 0);
        }

        vTaskDelay(xInferencePeriod);
    }
}
