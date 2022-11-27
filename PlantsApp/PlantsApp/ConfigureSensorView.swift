//
//  ConfigureSensorView.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 26/11/2022.
//

import SwiftUI
import SwiftJWT

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
    @State private var sensorFamiliarName = ""
    @State private var serverUrl = ""
    @State private var serverAuthToken = ""
    
    @ObservedObject public var bleProvisioningViewModel: BleProvisioningViewModel
    
    var body: some View {
        Form {
            Section {
                TextField("Przyjazna nazwa", text: $sensorFamiliarName)
                TextField("Adres url servera", text: $serverUrl)
                TextField("Token autoryzacyjny", text: $serverAuthToken)
                Button("Generuj token") {
                    serverAuthToken = bleProvisioningViewModel.deviceId
                }
            } header: {
                Text("Dane podstawowe")
            } footer: {
                if !bleProvisioningViewModel.basicSettingsResponse.isEmpty {
                    Text(bleProvisioningViewModel.basicSettingsResponse)
                        .font(.footnote)
                }
            }
            Button("Wyślij") {
                bleProvisioningViewModel.sendBasicConfigToDevice(
                    BasicConfiguration(sensorFamiliarName: sensorFamiliarName, serverUrl: serverUrl, serverAuthToken: serverAuthToken))
            }
            .frame(maxWidth: .infinity, alignment: .center)
            .disabled(sensorFamiliarName.isEmpty || serverUrl.isEmpty || serverAuthToken.isEmpty)

            Section {
                TextField("SSID", text: $wifiSsid)
                TextField("Hasło", text: $wifiPassword)
            } header: {
                Text("Konfiguracja wifi")
            } footer: {
                if !bleProvisioningViewModel.wifiSettingsResponse.isEmpty {
                    Text(bleProvisioningViewModel.wifiSettingsResponse)
                        .font(.footnote)
                }
            }
            Button("Wyślij") {
                bleProvisioningViewModel.provisionDevice(ssid: wifiSsid, passphrase: wifiPassword)
            }
            .frame(maxWidth: .infinity, alignment: .center)
            .disabled(wifiSsid.isEmpty || wifiPassword.isEmpty)
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
