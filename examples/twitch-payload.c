#define JP_IMPLEMENTATION
#include "../jp.h"

#include <stdio.h>

void *custom_malloc(unsigned long long int size)
{
    return malloc(size);
}

int main(void)
{
    const char *input = "{\n\
    \"_id\": 6969,\n\
    \"bio\": \":)\",\n\
    \"created_at\": \"2013-06-03T19:12:02Z\",\n\
    \"display_name\": \"dallas\",\n\
    \"email\": \"email-address@provider.com\",\n\
    \"email_verified\": true,\n\
    \"logo\": \"https://www.test-url.net/abc/defg\",\n\
    \"name\": \"Ciremun\",\n\
    \"notifications\": {\n\
        \"email\": false,\n\
        \"push\": true\n\
    },\n\
    \"partnered\": false,\n\
    \"twitter_connected\": false,\n\
    \"type\": \"staff\",\n\
    \"updated_at\": \"2069-12-14T01:01:44Z\"\n\
}";

    printf("%s\n", input);

    JMemory memory;
    memory.alloc = custom_malloc;
    JParser parser = json_init(&memory, input);
    JValue json = json_parse(&parser);

    if (json.type == JSON_OBJECT)
    {
        JValue id = json_get(&json.object, "_id");
        JValue name = json_get(&json.object, "name");
        JValue notifications = json_get(&json.object, "notifications");

        if (id.type == JSON_NUMBER)
            printf("id.number: %lld\n", id.number);
        if (name.type == JSON_STRING)
        {
            printf("name.string: %s\n", name.string);
            printf("name.length: %llu\n", name.length);
        }
        if (notifications.type == JSON_OBJECT)
        {
            JValue push = json_get(&notifications.object, "push");
            if (push.type == JSON_BOOL)
                printf("push.boolean: %d\n", push.boolean);
        }
    }

    return 0;
}
