//
//  ConfigureSensorView.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 26/11/2022.
//

import SwiftUI

struct WifiConfiguration: Codable {
    var ssid: String
    var passphre: String
}

struct BasicConfiguration: Codable {
    var sensorFamiliarName: String
    var serverUrl: String
    var serverAuthToken: String
}

struct ConfigureSensorView: View {
    @State private var wifiSsid = ""
    @State private var wifiPassword = ""
    
    @ObservedObject public var bluetoothViewModel: BluetoothViewModel
    
    var body: some View {
        Form {
            Section {
                TextField("SSID", text: $wifiSsid)
                TextField("Hasło", text: $wifiPassword)
            } header: {
                Text("Konfiguracja wifi")
            }
            Button("Wyślij") {
                bluetoothViewModel.provisionDevice(ssid: wifiSsid, password: wifiPassword)
            }
            .frame(maxWidth: .infinity, alignment: .center)
            .disabled(wifiSsid.isEmpty || wifiPassword.isEmpty)
            
            if !bluetoothViewModel.wifiSettingsResponse.isEmpty {
                Text(bluetoothViewModel.wifiSettingsResponse)
                    .frame(maxWidth: .infinity, alignment: .center)
                    .foregroundColor(bluetoothViewModel.wifiSettingsApplied ? .green : .red)
            }
        }
        .navigationTitle("Konfiguracja czujnika")
        .navigationBarTitleDisplayMode(.inline)
    }
}

struct ConfigureSensorView_Previews: PreviewProvider {
    static var previews: some View {
        ConfigureSensorView(bluetoothViewModel: BluetoothViewModel())
    }
}
