//
//  AddPlantView.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 12/11/2022.
//

import SwiftUI

struct AddPlantView: View {
    @State private var plantName = ""
    @State private var maxHumidity = 0
    @State private var maxInsolation = 0
    @State private var selectedSensorId = 0
    
    @State private var showingAddSensorSheet = false

    var percentages = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
    @State private var sensors: [Sensor] = []
    
    var body: some View {
        NavigationView{
            Form {
                Section(header: Text("Nadaj nazwę")){
                    TextField("Przyjazna nazwa", text: $plantName)
                }
                Section(header: Text("Parametry")) {
                    Picker("Maksymalna wilgotość gleby", selection: $maxHumidity) {
                            ForEach(percentages, id: \.self) {
                                Text($0, format: .percent)
                            }
                        }
                    Picker("Maksymalne nasłonecznienie", selection: $maxInsolation) {
                            ForEach(percentages, id: \.self) {
                                Text($0, format: .percent)
                            }
                        }
                }
                Section{
                    Picker("Wybierz czujnik", selection: $selectedSensorId) {
                        ForEach(sensors, id: \.id) { sensor in
                            Text(sensor.familiarName)
                        }
                    }
                    .task {
                        sensors = await getSensors()
                    }
                    .pickerStyle(.inline)
                    .labelsHidden()
                } header: {
                    HStack{
                        Text("Czujnik")
                        Spacer()
                        NavigationLink {
                            AddSensorView()
                        } label: {
                            Image(systemName: "plus")
                        }
                    }
                }
                
            }
            .navigationTitle("Dodaj roślinę")
            .navigationBarTitleDisplayMode(.inline)
        }
    }
}

struct AddPlantView_Previews: PreviewProvider {
    static var previews: some View {
        AddPlantView()
    }
}
