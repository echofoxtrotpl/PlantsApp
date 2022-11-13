//
//  PlantDetailedView.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 23/10/2022.
//

import SwiftUI

struct PlantDetailedView: View {
    var plant: Plant
    @State private var isTempExceeded = false
    @State private var record: Record = Record(id: 0, temperature: 20, humidity: 0.3, updatedAt: Date.now, sensorId: 1)
    
    var body: some View {
        ZStack{
            Color(red: 182, green: 224, blue: 189)
                .ignoresSafeArea()
                .task {
                    record = await getCurrentRecord(sensorId: plant.sensorId)
                    isTempExceeded = record.temperature > plant.maxTemperature || record.temperature < plant.minTemperature
                }
            VStack{
                HStack{
                    ZStack{
                        Image("plant")
                            .resizable()
                            .blur(radius: 3)
                            .scaledToFill()
                        HStack{
                            VStack (alignment: .leading){
                                HStack{
                                    Text("Data ostatniej aktualizacji: ")
                                        .font(.caption)
                                    Text(record.updatedAt, format: .dateTime)
                                        .font(.caption)
                                }
                                Text(plant.familiarName)
                                    .font(.largeTitle)
                                    .bold()
                                Text(plant.location)
                            }
                            .shadow(radius: 3)
                            .padding(30)
                            Spacer()
                        }
                        .foregroundColor(.white)
                    }
                }
                .frame(height: 270)
                HStack{
                    ScrollView{
                        VStack(alignment: .leading, spacing: 30){
                            HStack{
                                Text("Aktualne warunki")
                                    .font(.title2)
                                    .bold()
                                    .padding(.bottom, 5)
                            }
                            HStack{
                                Text("Nawodnienie")
                                Spacer()
                                ProgressChart(progress: record.humidity, maxValue: plant.maxHumidity, minValue: plant.minHumidity)
                                    .frame(width: 90, height: 90)
                            }
                            HStack{
                                Text("Temperatura")
                                Spacer()
                                //to change
                                if isTempExceeded {
                                    Image(systemName: "exclamationmark.triangle")
                                        .foregroundColor(.red)
                                }
                                Text(String(format: "%.1f°C", record.temperature))
                                    .foregroundColor(isTempExceeded ? .red : .black)
                                
                            }
                        }
                        .padding(30)
                        VStack(alignment: .leading, spacing: 30){
                            HStack{
                                Text("Idealne warunki")
                                    .font(.title2)
                                    .bold()
                                    .padding(.bottom, 5)
                            }
                            HStack{
                                Text("Nawodnienie")
                                Spacer()
                                Text("\(String(format: "%.0f", plant.minHumidity * 100))% - \(String(format: "%.0f", plant.maxHumidity * 100))%")
                            }
                            HStack{
                                Text("Temperatura")
                                Spacer()
                                Text("\(String(format: "%.1f°C", plant.minTemperature)) - \(String(format: "%.1f°C", plant.maxTemperature))")
                            }
                        }
                        .padding(30)
                    }
                }
                .background(
                    RoundedCorner(radius: 30, corners: [.topLeft, .topRight])
                        .fill(.white)
                        .ignoresSafeArea()
                )
            }
        }
    }
}

struct PlantDetailedView_Previews: PreviewProvider {
    static var previews: some View {
        PlantDetailedView(plant: Plant(id: 1, familiarName: "kwiat", location: "kuchnia", maxHumidity: 0.7, minHumidity: 0.2, maxTemperature: 22, minTemperature: 18, sensorId: 1))
    }
}
