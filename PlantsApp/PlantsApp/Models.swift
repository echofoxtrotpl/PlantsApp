//
//  Plant.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 13/11/2022.
//

import Foundation

struct Plant: Codable {
    var id: Int
    var familiarName: String
    var location: String
    var maxHumidity: Float
    var minHumidity: Float
    var maxTemperature: Float
    var minTemperature: Float
    var sensorId: Int
}

struct Sensor: Codable {
    var id: Int
    var familiarName: String
    var bluetoothName: String
    var plantId: Int
}

struct Record: Codable {
    var id: Int
    var temperature: Float
    var humidity: Float
    var updatedAt: Date
    var sensorId: Int
}
