//
//  HttpClient.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 13/11/2022.
//

import Foundation

final class HttpClient: ObservableObject {
    @Published var baseUrl = "http://130.61.149.252:8880/heatingsystem/api/iot/"
    @Published var jwtToken = ""
    @Published var userId = ""
    
    func getPlants() async -> [Plant] {
        var response: [Plant] = []
        
        let url = URL(string: baseUrl + "user/\(userId)/plants")!
        
        do {
            let (data, _) = try await URLSession.shared.data(from: url)
            
            if let decodedResponse = try? JSONDecoder().decode(PlantsResponseDto.self, from: data) {
                response = decodedResponse.plants
            }
        } catch {
            print("Invalid data")
        }
        
        return response
    }

    func addPlant(_ plant: Plant) async -> Bool{
        let url = URL(string: baseUrl + "user/\(userId)/plant")!
        
        var request = URLRequest(url: url)
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        request.setValue("Bearer \(jwtToken)", forHTTPHeaderField: "Authorization")
        request.httpMethod = "POST"
        
        do {
            let encoded = try JSONEncoder().encode(plant)
            let (_, response) = try await URLSession.shared.upload(for: request, from: encoded)
            guard let httpResponse = response as? HTTPURLResponse, httpResponse.statusCode == 201 else {
                return false
            }
            return true
        } catch {
            print("Add plant failed.")
            return false
        }
    }

    func removePlant(_ plantId: Int) async -> Void {
        let url = URL(string: baseUrl + "user/\(userId)/plant/\(plantId)")!
        
        var request = URLRequest(url: url)
        request.httpMethod = "DELETE"
        
        URLSession.shared.dataTask(with: request) { (data, res, err) in
            
        }.resume()
    }
    
    func login(dto: LoginDto) async -> Bool {
        let url = URL(string: baseUrl + "login")!
        
        var request = URLRequest(url: url)
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        request.httpMethod = "POST"
        
        do {
            let encoded = try JSONEncoder().encode(dto)
            let (data, response) = try await URLSession.shared.upload(for: request, from: encoded)
            guard let httpResponse = response as? HTTPURLResponse, httpResponse.statusCode == 200 else {
                return false
            }
            let decoded = try JSONDecoder().decode(LoginResponseDto.self, from: data)
            DispatchQueue.main.async {
                self.jwtToken = decoded.accessToken
                self.userId = decoded.userId
            }
            return true
        } catch {
            print("Login failed.")
            return false
        }
    }
    
    func signup(dto: SignupDto) async -> Bool {
        let url = URL(string: baseUrl + "register")!
        
        var request = URLRequest(url: url)
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        request.httpMethod = "POST"
        
        do {
            let encoded = try JSONEncoder().encode(dto)
            let (_, response) = try await URLSession.shared.upload(for: request, from: encoded)
            guard let httpResponse = response as? HTTPURLResponse, httpResponse.statusCode == 201 else {
                return false
            }
            return true
        } catch {
            print("Signup failed.")
            return false
        }
    }
}



