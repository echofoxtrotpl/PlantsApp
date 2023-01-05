//
//  AddPlantView.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 12/11/2022.
//

import SwiftUI

struct AddPlantView: View {
    @Environment(\.dismiss) var dismiss
    
    @State private var plantName = ""
    @State private var maxHumidity = 100
    @State private var minHumidity = 0
    @State private var maxTemperature: Double = 0.0
    @State private var minTemperature: Double = 0.0
    @State private var location = ""
    
    // Sensor
    @State private var selectedSensorName = ""
    @StateObject private var bleProvisioningViewModel = BleProvisioningViewModel()
    @ObservedObject var mqttManager: MQTTManager
    
    var body: some View {
        NavigationView{
            Form {
                Section(header: Text("O roślinie")){
                    TextField("Przyjazna nazwa", text: $plantName)
                    TextField("Lokalizacja", text: $location)
                }
                Section(header: Text("Parametry")) {
                    Picker("Minimalna wilgotość", selection: $minHumidity) {
                        ForEach(0...maxHumidity, id: \.self) {
                            Text($0, format: .percent)
                        }
                    }
                    Picker("Maksymalna wilgotość", selection: $maxHumidity) {
                        ForEach(minHumidity...100, id: \.self) {
                            Text($0, format: .percent)
                        }
                    }
                    Picker("Minimalna temperatura", selection: $minTemperature) {
                        ForEach(Array(stride(from: -20.0, to: maxTemperature, by: 0.1)), id: \.self) {
                            Text($0, format: .number)
                        }
                    }
                    Picker("Maksymalna temperatura", selection: $maxTemperature) {
                        ForEach(Array(stride(from: minTemperature, to: 50.0, by: 0.1)), id: \.self) {
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
            .toolbar {
                Button("Zapisz"){
                    Task {
                        let newPlant = createPlant()
                        let success = await addPlant(newPlant)
                        if (success) {
                            mqttManager.subscribe(topic: "\(newPlant.sensorName)/records")
                            dismiss()
                        }
                    }
                }
                .frame(maxWidth: .infinity, alignment: .center)
                .disabled(selectedSensorName.isEmpty || plantName.isEmpty || !bleProvisioningViewModel.wifiSettingsApplied
                          || location.isEmpty)
            }
        }
    }
    
    func createPlant() -> Plant {
        return Plant(familiarName: plantName, location: location, maxHumidity: maxHumidity, minHumidity: minHumidity, maxTemperature: maxTemperature, minTemperature: minTemperature, sensorName: selectedSensorName)
    }
}

struct AddPlantView_Previews: PreviewProvider {
    static var previews: some View {
        AddPlantView(mqttManager: MQTTManager())
    }
}
