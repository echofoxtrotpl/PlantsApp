//
//  PlantDemoCard.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 23/10/2022.
//

import SwiftUI

struct PlantDemoCard: View {
    var plant: Plant

    var body: some View {
        HStack{
            Spacer()
            NavigationLink {
                PlantDetailedView(plant: plant)
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
                                TagPill(tagContent: "trza podlać")
                                TagPill(tagContent: "za gorąco")
                            }
                            .foregroundColor(.white)
                            .padding()
                            Spacer()
                            VStack{
                                Spacer()
                                Image(systemName: "arrow.forward")
                                    .foregroundColor(.white)
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
        PlantDemoCard(plant: Plant(id: 1, familiarName: "kwiat", location: "kuchnia", maxHumidity: 70, minHumidity: 20, maxTemperature: 22, minTemperature: 18, sensorId: 1))
    }
}
