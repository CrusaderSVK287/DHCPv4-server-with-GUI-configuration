#include <stdio.h>
#include <stdlib.h>

#include <cJSON_Utils.h>
#include <cclog.h>
#include <cJSON.h>

int main(int argc, char *argv[])
{
        cclogger_init(LOGGING_SINGLE_FILE, "/tmp/log", "progname");
        cclogger_uninit();

        // Creating a JSON object
        cJSON *root = cJSON_CreateObject();
        if (root == NULL) {
                printf("Error creating JSON object!\n");
                return 1;
        }

        // Adding key-value pairs to the JSON object
        cJSON_AddItemToObject(root, "name", cJSON_CreateString("John Doe"));
        cJSON_AddItemToObject(root, "age", cJSON_CreateNumber(30));
        cJSON_AddItemToObject(root, "isStudent", cJSON_CreateBool(0));

        // Convert JSON object to string and print
        char *jsonString = cJSON_Print(root);
        if (jsonString == NULL) {
                printf("Error converting JSON to string!\n");
                cJSON_Delete(root);
                return 1;
        }
    
        printf("JSON Object:\n%s\n", jsonString);

        // Cleanup
        cJSON_Delete(root);
        free(jsonString);

        return 0;
}
