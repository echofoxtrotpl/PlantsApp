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
    
    @ObservedObject public var bleProvisioningViewModel: BleProvisioningViewModel
    
    var body: some View {
        Form {
            Section {
                TextField("SSID", text: $wifiSsid)
                TextField("Hasło", text: $wifiPassword)
            } header: {
                Text("Konfiguracja wifi")
            }
            Button("Wyślij") {
                bleProvisioningViewModel.provisionDevice(ssid: wifiSsid, passphrase: wifiPassword)
            }
            .frame(maxWidth: .infinity, alignment: .center)
            .disabled(wifiSsid.isEmpty || wifiPassword.isEmpty)
            
            if !bleProvisioningViewModel.wifiSettingsResponse.isEmpty {
                Text(bleProvisioningViewModel.wifiSettingsResponse)
                    .frame(maxWidth: .infinity, alignment: .center)
                    .foregroundColor(bleProvisioningViewModel.wifiSettingsApplied ? .green : .red)
            }
        }
        .navigationTitle("Konfiguracja czujnika")
        .navigationBarTitleDisplayMode(.inline)
    }
}

struct ConfigureSensorView_Previews: PreviewProvider {
    static var previews: some View {
        ConfigureSensorView(bleProvisioningViewModel: BleProvisioningViewModel())
    }
}
