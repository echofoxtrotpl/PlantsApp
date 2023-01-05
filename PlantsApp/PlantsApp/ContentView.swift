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
                VStack{
                    ZStack{
                        Rectangle()
                            .foregroundColor(Color(.systemGray6))
                        HStack{
                            Image(systemName: "magnifyingglass")
                            TextField("Wyszukaj...", text: $searchText)
                        }
                        .foregroundColor(.gray)
                        .padding(.leading, 13)
                    }
                    .frame(height: 40)
                    .cornerRadius(13)
                    .padding()
                    ScrollView {
                        ForEach(plants, id: \.id) { plant in
                            PlantDemoCard(plant: plant, mqttManager: mqttManager)
                        }
                    }
                    .task{
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
