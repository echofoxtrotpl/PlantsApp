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
                    ScrollView() {
                        ForEach(plants, id: \.id) { plant in
                            PlantDemoCard(plant: plant)
                        }
                        .task {
                            plants = await getPlants()
                        }
                    }
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
                    AddPlantView()
                }
            
        }
        .accentColor(.white)
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
