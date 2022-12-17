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

class Record: ObservableObject {
    var id: Int = 0
    @Published var temperature: Double = 0.0
    @Published var humidity: Int = 0
    @Published var updatedAt: Date = Date.now
    @Published var sensorName: String = ""
}

struct MqttMessage: Codable {
    var sensorName: String
    var humidity: Int?
    var temperature: Double?
}
