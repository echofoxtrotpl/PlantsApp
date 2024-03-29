//
//  PlantDemoCard.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 23/10/2022.
//

import SwiftUI

struct PlantDemoCard: View {
    var plant: Plant
    @ObservedObject var mqttManager: MQTTManager
    @ObservedObject var httpClient: HttpClient
    @AppStorage private var lastRecord: CodableWrapper<RecordStruct>
    
    init(plant: Plant, mqttManager: MQTTManager, httpClient: HttpClient){
        self.plant = plant
        self.mqttManager = mqttManager
        self.httpClient = httpClient
        self._lastRecord = AppStorage(wrappedValue: .init(value: RecordStruct(temperature: 0.0, humidity: 0, updatedAt: Date(), sensorName: plant.sensorName)), plant.sensorName)
    }
    
    private var record: RecordStruct {
        get {
            mqttManager.records[plant.sensorName] ?? lastRecord.value
        }
    }
    private var isTempExceeded: Bool {
        get {
            record.temperature > plant.maxTemperature ||
            record.temperature < plant.minTemperature
        }
    }
    private var isHumidityExceeded: Bool {
        get {
            record.humidity > plant.maxHumidity ||
            record.humidity < plant.minHumidity
        }
    }

    var body: some View {
        HStack{
            Spacer()
            NavigationLink {
                PlantDetailedView(mqttManager: mqttManager, plant: plant, httpClient: httpClient)
            } label: {
                ZStack{
                    RoundedCorner(radius: 45, corners: [.bottomLeft, .topLeft])
                        .fill(Color(red: 182, green: 224, blue: 189))
                    HStack {
                        Image("plant")
                            .resizable()
                            .scaledToFit()
                        HStack {
                            VStack(alignment: .leading, spacing: 5){
                                Text(plant.familiarName)
                                    .font(.title)
                                Text(plant.location)
                                    .font(.caption)
                                Spacer()
                                if isTempExceeded {
                                    TagPill(tagContent: "temperatura")
                                }
                                if isHumidityExceeded {
                                    TagPill(tagContent: "wilgotność")
                                }
                            }
                            .foregroundColor(.white)
                            .padding()
                            Spacer()
                            VStack{
                                Image(systemName: "arrow.forward")
                                    .foregroundColor(.white)
                                    .padding(.vertical)
                                    .padding(.trailing)
                                Spacer()
                                Button{
                                    Task {
                                        await httpClient.removePlant(_: plant.id!)
                                    }
                                } label: {
                                    Image(systemName: "trash")
                                        .foregroundColor(.red)
                                }
                                .padding(.vertical)
                                .padding(.trailing)
                            }
                        }
                    }
                }
                .frame(width: 370, height: 150)
            }
        }
        .padding(.vertical, 10)
    }
}

struct PlantDemoCard_Previews: PreviewProvider {
    static var previews: some View {
        PlantDemoCard(plant: Plant(familiarName: "kwiat", location: "kuchnia", maxHumidity: 70, minHumidity: 20, maxTemperature: 22, minTemperature: 18, sensorName: "", pushIntervalInMinutes: 20), mqttManager: MQTTManager(), httpClient: HttpClient())
    }
}
