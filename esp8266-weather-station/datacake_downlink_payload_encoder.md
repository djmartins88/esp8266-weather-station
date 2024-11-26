In Datacake Device Configuration set the following code if you want to make a PATCH request to some server:
```
function Encoder(device, measurements) {
        return {
            method: "PATCH",
            url: '<YOUR-API-URL>',
            // Optional query parameters as object
            queryParameters: {
                "id": "eq.1"
            },
            // Optional headers
            headers: {
                "content-type": "application/json",
                // your API auth headers as specified by api
                "Authorization": "Bearer <YOUR-JWT>"
            },
            // The body of the request, applicable for POST, PUT, PATCH methods. Can be an object or string.
            body: {
                "TEMPERATURE_TRESHOLD": measurements['TEMP_TRESHOLD']['value'],
                "HUMIDITY_TRESHOLD": measurements['HUMIDITY_TRESHOLD']['value']
            }
        };
    }
```
Basically it will send this request when one of those fields is changed. You can use it to upload the treshold values in the database, for example.