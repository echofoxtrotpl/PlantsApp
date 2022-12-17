//
//  HttpClient.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 13/11/2022.
//

import Foundation

let baseUrl = "http://192.168.20.101:3000/"

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

func addPlant(_ plant: Plant) async -> Bool{
    guard let url = URL(string: baseUrl + "plants") else {
        print("Invalid URL")
        return false
    }
    
    var request = URLRequest(url: url)
    request.setValue("application/json", forHTTPHeaderField: "Content-Type")
    request.httpMethod = "POST"
    
    do {
        let encoded = try JSONEncoder().encode(plant)
        let (_, _) = try await URLSession.shared.upload(for: request, from: encoded)
        return true
    } catch {
        print("Checkout failed.")
        return false
    }
}

func removePlant(_ plantId: Int) async -> Void{
    guard let url = URL(string: baseUrl + "plants/\(plantId)") else {
        print("Invalid URL")
        return
    }
    
    var request = URLRequest(url: url)
    request.httpMethod = "DELETE"
    
    URLSession.shared.dataTask(with: request) { (data, res, err) in
        
    }.resume()
}

