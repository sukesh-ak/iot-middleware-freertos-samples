/******************************************************************************
 * FunctionName : show_sensor_task
 * Description  : Task for sending temp, humidity, and brightness 
 *                to OLED/SSD1306 - Lower Task priority 
 * Parameters   : pvParameter
 * Returns      : none
*******************************************************************************/
void show_sensor_task(void *pvParameter)
{
    //If want to add MPU sensor and button - Insert Your Code Here
    
    oled_write_temp_header();

    while(1)
    {
        oled_write_temp_data();
        vTaskDelay(ONE_SECOND_DELAY *3);
    }
    
    vTaskDelete(NULL);  
}