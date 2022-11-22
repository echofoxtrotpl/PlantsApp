//
//  AddSensorView.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 12/11/2022.
//

import SwiftUI

struct WifiConfig: Codable {
    var ssid: String
    var password: String
}

struct AddSensorView: View {
    @State private var sensorFamiliarName = ""
    @State private var selectedSensorName = ""
    @State private var wifiSsid = ""
    @State private var wifiPassword = ""
    @ObservedObject private var bluetoothViewModel = BluetoothViewModel()
    
    var body: some View {
        Form {
            Section(header: Text("Nadaj nazwę")){
                TextField("Przyjazna nazwa", text: $sensorFamiliarName)
            }
            if bluetoothViewModel.peripheralNames.count > 0 {
                Picker("Wybierz czujnik", selection: $selectedSensorName) {
                    ForEach(bluetoothViewModel.peripheralNames, id: \.self) { peripheral in
                        Text(peripheral)
                    }
                }
                if !selectedSensorName.isEmpty && !bluetoothViewModel.connected {
                    Button("Nawiąż połączenie") {
                        bluetoothViewModel.connectToPeripheral(selectedSensorName)
                    }
                }
            } else {
                Text("Wyszukiwanie urządzeń...")
            }
            
            if bluetoothViewModel.connected {
                Section("Konfiguracja") {
                    Text("Połączono")
                        .foregroundColor(.green)

                    TextField("SSID", text: $wifiSsid)
                    TextField("Hasło", text: $wifiPassword)

                    Button("Wyślij") {
                        bluetoothViewModel.send(encodeJson(WifiConfig(ssid: wifiSsid, password: wifiPassword)))
                    }
                }
            }
            
        }
        .navigationTitle("Dodaj czujnik")
        .navigationBarTitleDisplayMode(.inline)
    }
    
    func encodeJson(_ value: Codable) -> String {
        let encoder = JSONEncoder()
        if let json = try? encoder.encode(value) {
            return String(decoding: json, as: UTF8.self)
        }
        
        return ""
    }
}

struct AddSensorView_Previews: PreviewProvider {
    static var previews: some View {
        AddSensorView()
    }
}
