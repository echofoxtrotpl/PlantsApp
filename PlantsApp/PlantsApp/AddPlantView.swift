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
    @State private var minHumidity = 0
    @State private var maxTemperature = 0
    @State private var minTemperature = 0
    @State private var selectedSensorId = 0
    
    @State private var showingAddSensorSheet = false
    
    // Sensor
    @State private var selectedSensorName = ""
    @StateObject private var bleProvisioningViewModel = BleProvisioningViewModel()

    var percentages = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
    @State private var sensors: [Sensor] = []
    
    var body: some View {
        NavigationView{
            Form {
                Section(header: Text("Nadaj nazwę")){
                    TextField("Przyjazna nazwa", text: $plantName)
                }
                Section(header: Text("Parametry")) {
                    Picker("Minimalna wilgotość gleby", selection: $minHumidity) {
                        ForEach(percentages, id: \.self) {
                            Text($0, format: .percent)
                        }
                    }
                    Picker("Maksymalna wilgotość gleby", selection: $maxHumidity) {
                        ForEach(percentages, id: \.self) {
                            Text($0, format: .percent)
                        }
                    }
                    Picker("Minimalna temperatura", selection: $minTemperature) {
                        ForEach(-20...maxTemperature, id: \.self) {
                            Text($0, format: .number)
                        }
                    }
                    Picker("Maksymalna temperatura", selection: $maxTemperature) {
                        ForEach(minTemperature...50, id: \.self) {
                            Text($0, format: .number)
                        }
                    }
                }
                
                if bleProvisioningViewModel.foundDevicesNames.count > 0 {
                    Picker(selection: $selectedSensorName) {
                        ForEach(bleProvisioningViewModel.foundDevicesNames, id: \.self) { device in
                            Text(device)
                        }
                    } label: {
                        HStack {
                            Text("Wybierz czujnik")
                            Spacer()
                            if bleProvisioningViewModel.connected {
                                Button("Rozłącz") {
                                    bleProvisioningViewModel.disconnect()
                                }
                                .foregroundColor(.red)
                            } else {
                                Button{
                                    bleProvisioningViewModel.scanForDevices()
                                } label: {
                                    Image(systemName: "arrow.counterclockwise")
                                        .foregroundColor(.blue)
                                }
                            }
                        }
                    }
                    .pickerStyle(.inline)
                    
                    if bleProvisioningViewModel.connected {
                        NavigationLink {
                            ConfigureSensorView(bleProvisioningViewModel: bleProvisioningViewModel)
                        } label: {
                            Text("Konfiguruj czujnik")
                        }
                    }
                    
                    if !selectedSensorName.isEmpty && !bleProvisioningViewModel.connected {
                        Button("Nawiąż połączenie") {
                            bleProvisioningViewModel.connectToDevice(selectedSensorName)
                        }
                        .frame(maxWidth: .infinity, alignment: .center)
                    }
                } else {
                    Section("Wybierz czujnik") {
                        Text("Wyszukiwanie urządzeń ...")
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
