//
//  TagPill.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 23/10/2022.
//

import SwiftUI

struct TagPill: View {
    var tagContent = "jest git"
    
    var body: some View {
        HStack{
            Image(systemName: "exclamationmark.triangle")
            Text(tagContent)
                .font(.caption)
                .bold()
        }
        .padding(.horizontal)
        .padding(.vertical, 4)
        .foregroundColor(.white)
        .background(
            Capsule()
                .fill(.yellow)
        )
    }
}

struct TagPill_Previews: PreviewProvider {
    static var previews: some View {
        TagPill()
    }
}
