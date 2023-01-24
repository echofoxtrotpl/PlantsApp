//
//  Plant.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 13/11/2022.
//

import Foundation

struct Plant: Codable {
    var id: Int?
    var familiarName: String
    var location: String
    var maxHumidity: Int
    var minHumidity: Int
    var maxTemperature: Double
    var minTemperature: Double
    var sensorName: String
}

struct RecordStruct: Codable {
    var temperature: Double
    var humidity: Int
    var updatedAt: Date
    var sensorName: String
}

struct CodableWrapper<Value: Codable> {
    var value: Value
}

class Record: ObservableObject {
    var id: Int = 0
    @Published var temperature: Double = 0.0
    @Published var humidity: Int = 0
    @Published var updatedAt: Date = Date.now
    @Published var sensorName: String = ""
}

struct MqttMessage: Codable {
    var humidity: Double
    var temperature: Double
    var updatedAt: Double
}

struct WifiCredentials: Codable {
    var ssid: String
    var password: String
}

struct LoginDto: Codable {
    var email: String
    var password: String
}

struct SignupDto: Codable {
    var email: String
    var password: String
    var firstName: String
    var lastName: String
}

struct LoginResponseDto: Codable {
    var accessToken: String
    var userId: String
}

struct PlantsResponseDto: Codable {
    var plants: [Plant]
}
