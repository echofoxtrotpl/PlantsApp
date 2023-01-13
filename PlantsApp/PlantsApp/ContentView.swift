//
//  ContentView.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 23/10/2022.
//

import SwiftUI

struct ContentView: View {
    @State private var searchText = ""
    @State private var plants: [Plant] = []
    @State private var serverUrl = "http://192.168.20.100:3000/"
    @StateObject var mqttManager: MQTTManager = MQTTManager()
    @StateObject var httpClient: HttpClient = HttpClient()
    
    var body: some View {
        NavigationView {
            ScrollView {
                if plants.count > 0 {
                    ForEach(plants, id: \.id) { plant in
                        PlantDemoCard(plant: plant, mqttManager: mqttManager, httpClient: httpClient)
                    }
                } else {
                    Text("Pociągnij w dół aby odświeżyć")
                        .font(.footnote)
                    TextField("Url servera", text: $serverUrl)
                        .multilineTextAlignment(.center)
                        .onChange(of: serverUrl) { newValue in
                            httpClient.baseUrl = serverUrl
                        }
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
            }
            .padding(.top, 10)
        }
        .accentColor(.black)
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

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
