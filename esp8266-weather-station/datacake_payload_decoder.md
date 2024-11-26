In Datacake Device Configuration set the following code as HTTP Payload Decoder:
```
function Decoder(request) {
    
    // First, parse the request body into a JSON object to process it
    var payload = JSON.parse(request.body)

    // Extract the serial number from the incoming data
    var serialNumber = payload.device

    // Get the Unix timestamp in seconds to use as the ingestion timestamp
    var timestamp = Math.floor(Date.now() / 1000);

    var result = Object.keys(payload).map(function(key) {
      if (key !== "device") {
        return {
          device: payload.device,
          field: key.toUpperCase(),
          value: payload[key],
          timestamp: timestamp // timestamps are optional but can be useful for data tracking and auditing
        };
      }
    });

    return result
}
```
Basically it will automatically decode your data fields from your payload, and store them in Datacake data.