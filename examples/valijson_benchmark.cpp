#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <valijson/adapters/nlohmann_json_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>
#include <valijson/utils/nlohmann_json_utils.hpp>

namespace fs = std::filesystem;

namespace {

void printUsage(const std::string &programName)
{
    std::cerr << "Usage: " << programName
              << " <iterations> <schema> <document|directory> [document|directory]..." << std::endl;
}

bool gatherDocumentPaths(const fs::path &input, std::vector<fs::path> &documents)
{
    std::error_code ec;
    const fs::file_status status = fs::status(input, ec);
    if (ec) {
        std::cerr << "Failed to access '" << input.string() << "': " << ec.message() << std::endl;
        return false;
    }

    if (fs::is_regular_file(status)) {
        const fs::path resolved = fs::canonical(input, ec);
        if (ec) {
            std::cerr << "Failed to resolve path '" << input.string() << "': " << ec.message() << std::endl;
            return false;
        }
        documents.push_back(resolved);
        return true;
    }

    if (fs::is_directory(status)) {
        fs::recursive_directory_iterator dirIt(input, ec);
        if (ec) {
            std::cerr << "Failed to iterate directory '" << input.string() << "': "
                      << ec.message() << std::endl;
            return false;
        }

        const fs::recursive_directory_iterator end;
        while (dirIt != end) {
            const fs::directory_entry &entry = *dirIt;
            ec.clear();
            if (entry.is_regular_file(ec)) {
                if (ec) {
                    std::cerr << "Failed to inspect entry '" << entry.path().string()
                              << "': " << ec.message() << std::endl;
                    return false;
                }

                const fs::path resolved = fs::canonical(entry.path(), ec);
                if (ec) {
                    std::cerr << "Failed to resolve path '" << entry.path().string()
                              << "': " << ec.message() << std::endl;
                    return false;
                }

                documents.push_back(resolved);
            } else if (ec) {
                std::cerr << "Failed to inspect entry '" << entry.path().string()
                          << "': " << ec.message() << std::endl;
                return false;
            }

            dirIt.increment(ec);
            if (ec) {
                std::cerr << "Failed to continue iterating directory '" << input.string() << "': "
                          << ec.message() << std::endl;
                return false;
            }
        }

        return true;
    }

    std::cerr << "Skipping unsupported path '" << input.string() << "'" << std::endl;
    return false;
}

struct LoadedDocument {
    fs::path path;
    nlohmann::json document;
};

} // namespace

int main(int argc, char *argv[])
{
    if (argc < 4) {
        printUsage(argv[0]);
        return 1;
    }

    std::size_t iterations = 0;
    try {
        iterations = static_cast<std::size_t>(std::stoull(argv[1]));
    } catch (const std::exception &ex) {
        std::cerr << "Failed to parse iterations: " << ex.what() << std::endl;
        return 1;
    }

    if (iterations == 0) {
        std::cerr << "Iteration count must be greater than zero." << std::endl;
        return 1;
    }

    const fs::path schemaPath(argv[2]);

    nlohmann::json schemaDocument;
    if (!valijson::utils::loadDocument(schemaPath.string(), schemaDocument)) {
        std::cerr << "Failed to load schema document: " << schemaPath.string() << std::endl;
        return 1;
    }

    valijson::Schema schema;
    valijson::SchemaParser parser;
    try {
        valijson::adapters::NlohmannJsonAdapter schemaAdapter(schemaDocument);
        parser.populateSchema(schemaAdapter, schema);
    } catch (const std::exception &ex) {
        std::cerr << "Failed to parse schema: " << ex.what() << std::endl;
        return 1;
    }

    std::vector<fs::path> documentPaths;
    for (int i = 3; i < argc; ++i) {
        const fs::path candidate(argv[i]);
        if (!gatherDocumentPaths(candidate, documentPaths)) {
            return 1;
        }
    }

    if (documentPaths.empty()) {
        std::cerr << "No documents found to validate." << std::endl;
        return 1;
    }

    std::sort(documentPaths.begin(), documentPaths.end());
    documentPaths.erase(std::unique(documentPaths.begin(), documentPaths.end()), documentPaths.end());

    std::vector<LoadedDocument> documents;
    documents.reserve(documentPaths.size());

    for (const fs::path &path : documentPaths) {
        nlohmann::json document;
        if (!valijson::utils::loadDocument(path.string(), document)) {
            std::cerr << "Failed to load document: " << path.string() << std::endl;
            return 1;
        }
        documents.push_back({path, std::move(document)});
    }

    valijson::Validator validator(valijson::Validator::kStrongTypes);

    std::size_t failureCount = 0;

    const auto start = std::chrono::steady_clock::now();

    for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
        for (const LoadedDocument &loaded : documents) {
            valijson::adapters::NlohmannJsonAdapter documentAdapter(loaded.document);
            if (!validator.validate(schema, documentAdapter, nullptr)) {
                ++failureCount;
                valijson::ValidationResults results;
                validator.validate(schema, documentAdapter, &results);

#ifdef REPORT
                std::cerr << "Validation failed for " << loaded.path.string() << std::endl;
                valijson::ValidationResults::Error error;
                std::size_t errorIndex = 1;
                while (results.popError(error)) {
                    std::cerr << "  Error #" << errorIndex++ << ": " << error.description << std::endl;
                    std::cerr << "   @ " << error.jsonPointer << std::endl;
                }
#endif
            }
        }
    }

    const auto end = std::chrono::steady_clock::now();
    const auto elapsedSeconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    const std::size_t documentCount = documents.size();

    std::cout << "Validated " << (documentCount * iterations) << " documents in "
              << elapsedSeconds.count() << " seconds." << std::endl;
    std::cout << "Documents: " << documentCount << ", Iterations: " << iterations << " ("
              << ((double)(documentCount * iterations) / (double)(elapsedSeconds.count()))
              << " per second)" << std::endl;

    if (failureCount != 0) {
        std::cout << failureCount << " validation failure(s) encountered." << std::endl;
        return 1;
    }

    return 0;
}
