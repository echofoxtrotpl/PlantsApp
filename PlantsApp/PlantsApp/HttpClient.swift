//
//  HttpClient.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 13/11/2022.
//

import Foundation

let baseUrl = "http://127.0.0.1:3000/"

func getPlants() async -> [Plant] {
    var response: [Plant] = []
    
    guard let url = URL(string: baseUrl + "plants") else {
        print("Invalid URL")
        return response
    }
    
    do {
        let (data, _) = try await URLSession.shared.data(from: url)
        
        if let decodedResponse = try? JSONDecoder().decode([Plant].self, from: data) {
            response = decodedResponse
        }
    } catch {
        print("Invalid data")
    }
    
    return response
}

func getCurrentRecord(sensorId: Int) async -> Record {
    var response: Record = Record(id: 0, temperature: 0, humidity: 0, updatedAt: Date.now, sensorId: 0)
    
    guard let url = URL(string: baseUrl + "records") else {
        print("Invalid URL")
        return response
    }
    
    do {
        let (data, _) = try await URLSession.shared.data(from: url)
        
        let decoder = JSONDecoder()
        decoder.dateDecodingStrategy = .iso8601
        
        if let decodedResponse = try? decoder.decode([Record].self, from: data) {
            response = decodedResponse[0]
            print(response)
        }
    } catch {
        print("Invalid data")
    }
    
    return response
}

func getSensors() async -> [Sensor] {
    var response: [Sensor] = []
    
    guard let url = URL(string: baseUrl + "sensors") else {
        print("Invalid URL")
        return response
    }
    
    do {
        let (data, _) = try await URLSession.shared.data(from: url)
        
        if let decodedResponse = try? JSONDecoder().decode([Sensor].self, from: data) {
            response = decodedResponse
        }
    } catch {
        print("Invalid data")
    }
    
    return response
}
