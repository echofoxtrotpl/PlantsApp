//
//  PlantsView.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 23/01/2023.
//

import SwiftUI

struct PlantsView: View {
    @Binding var isLoggedIn: Bool
    @State private var searchText = ""
    @State private var plants: [Plant] = []
    @StateObject var mqttManager: MQTTManager = MQTTManager()
    @ObservedObject var httpClient: HttpClient
    
    var body: some View {
        ScrollView {
            if plants.count > 0 {
                ForEach(plants, id: \.id) { plant in
                    PlantDemoCard(plant: plant, mqttManager: mqttManager, httpClient: httpClient)
                }
            } else {
                Text("Pociągnij w dół aby odświeżyć")
                    .font(.footnote)
            }
        }
        .onAppear {
            Task{
                await loadPlants();
            }
        }
        .refreshable {
            plants = await httpClient.getPlants()
        }
        .navigationTitle("Twoje rośliny")
        .toolbar {
            NavigationLink {
                AddPlantView(mqttManager: mqttManager, httpClient: httpClient)
            } label: {
                HStack{
                    Image(systemName: "plus")
                    Text("Dodaj")
                }
                .foregroundColor(.black)
            }
            Button("Wyloguj się") {
                isLoggedIn = false
            }
        }
        .padding(.top, 10)
    }
    
    func loadPlants() async {
        plants = await httpClient.getPlants();
        mqttManager.initializeMQTT(
            host: "mqtt.eclipseprojects.io",
            identifier: UUID().uuidString,
            topics: plants.map{$0.sensorName}
        )
        mqttManager.connect()
    }
}

struct PlantsView_Previews: PreviewProvider {
    static var previews: some View {
        PlantsView(isLoggedIn: .constant(true), httpClient: HttpClient())
    }
}
