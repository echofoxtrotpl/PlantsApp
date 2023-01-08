//
//  ContentView.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 23/10/2022.
//

import SwiftUI

struct ContentView: View {
    @State private var searchText = ""
    @State private var showingAddPlantSheet = false
    @State private var plants: [Plant] = []
    @StateObject var mqttManager: MQTTManager = MQTTManager()
    
    var body: some View {
        NavigationView {
            ScrollView {
                if plants.count > 0 {
                    ForEach(plants, id: \.id) { plant in
                        PlantDemoCard(plant: plant, mqttManager: mqttManager)
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
                plants = await getPlants()
            }
            .navigationTitle("Twoje rośliny")
            .toolbar {
                Button {
                    showingAddPlantSheet = true
                } label: {
                    HStack{
                        Image(systemName: "plus")
                        Text("Dodaj")
                    }
                    .foregroundColor(.black)
                    
                }
            }
            .padding(.top, 10)
            .sheet(isPresented: $showingAddPlantSheet) {
                AddPlantView(mqttManager: mqttManager)
            }
        }
        .accentColor(.white)
    }
    
    func loadPlants() async {
        plants = await getPlants();
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
