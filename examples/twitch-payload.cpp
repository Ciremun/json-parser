#define JP_IMPLEMENTATION
#include "../jp.h"

#include <stdio.h>

int main()
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

    JValue json = json_parse(input);

    if (json.type == JSON_OBJECT)
    {
        JValue id = json["_id"];
        JValue name = json["name"];
        JValue notifications = json["notifications"];

        if (id.type == JSON_NUMBER)
            printf("id.number: %lld\n", id.number);
        if (name.type == JSON_STRING)
        {
            printf("name.string: %s\n", name.string.data);
            printf("name.length: %llu\n", name.string.length);
        }
        if (notifications.type == JSON_OBJECT)
        {
            JValue push = notifications["push"];
            if (push.type == JSON_BOOL)
                printf("push.boolean: %d\n", push.boolean);
        }
    }

    return 0;
}
