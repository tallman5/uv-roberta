#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

/*
g++ -o jsontest jsontest.cpp
*/

using json = nlohmann::json;

void serializeToJson(const json& data, const std::string& filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << data.dump(4);  // Write JSON to file with indentation of 4 spaces
        file.close();
        std::cout << "Serialization completed successfully.\n";
    } else {
        std::cerr << "Unable to open file: " << filename << std::endl;
    }
}

json deserializeFromJson(const std::string& filename) {
    std::ifstream file(filename);
    json data;

    if (file.is_open()) {
        file >> data;
        file.close();
        std::cout << "Deserialization completed successfully.\n";
    } else {
        std::cerr << "Unable to open file: " << filename << std::endl;
    }

    return data;
}

int main() {
    // Create some sample data
    json data;
    data["name"] = "John Doe";
    data["age"] = 30;
    data["city"] = "New York";

    // Serialize the JSON data
    serializeToJson(data, "data.json");

    // Deserialize the JSON data
    json deserializedData = deserializeFromJson("data.json");

    // Access the deserialized data
    std::string name = deserializedData["name"];
    int age = deserializedData["age"];
    std::string city = deserializedData["city"];

    // Output the deserialized data
    std::cout << "Name: " << name << std::endl;
    std::cout << "Age: " << age << std::endl;
    std::cout << "City: " << city << std::endl;

    return 0;
}
