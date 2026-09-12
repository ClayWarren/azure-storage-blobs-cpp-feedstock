#include <azure/storage/blobs/blob_client.hpp>
#include <azure/core/http/transport.hpp>
#include <azure/core/io/body_stream.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
class MockTransport final : public Azure::Core::Http::HttpTransport {
public:
    int calls = 0;
    std::unique_ptr<Azure::Core::Http::RawResponse> Send(
        Azure::Core::Http::Request& request, const Azure::Core::Context&) override {
        if (request.GetUrl().GetPath() != "container/blob")
            throw std::runtime_error("Unexpected blob path");
        ++calls;
        auto response = std::make_unique<Azure::Core::Http::RawResponse>(
            1, 1, Azure::Core::Http::HttpStatusCode::Ok, "OK");
        static const std::string body = "native-arm64-blob";
        response->SetHeader("Content-Length", std::to_string(body.size()));
        response->SetHeader("Content-Type", "application/octet-stream");
        response->SetHeader("ETag", "\"test-etag\"");
        response->SetHeader("Last-Modified", "Sat, 12 Sep 2026 00:00:00 GMT");
        response->SetHeader("x-ms-creation-time", "Sat, 12 Sep 2026 00:00:00 GMT");
        response->SetHeader("x-ms-blob-type", "BlockBlob");
        response->SetHeader("x-ms-server-encrypted", "true");
        response->SetBodyStream(std::make_unique<Azure::Core::IO::MemoryBodyStream>(
            reinterpret_cast<const uint8_t*>(body.data()), body.size()));
        return response;
    }
};
int main() {
    auto transport = std::make_shared<MockTransport>();
    Azure::Storage::Blobs::BlobClientOptions options;
    options.Transport.Transport = transport;
    Azure::Storage::Blobs::BlobClient client(
        "https://account.blob.core.windows.net/container/blob", options);
    auto response = client.Download();
    auto bytes = response.Value.BodyStream->ReadToEnd();
    if (std::string(bytes.begin(), bytes.end()) != "native-arm64-blob" ||
        transport->calls != 1) return 1;
    std::cout << "Installed Azure Blobs mocked download passed\n";
}
