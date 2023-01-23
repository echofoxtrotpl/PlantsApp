//
//  ContentView.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 23/10/2022.
//

import SwiftUI

struct ContentView: View {
    @State private var isLoggedIn = false
    @StateObject var httpClient: HttpClient = HttpClient()
    
    var body: some View {
        NavigationView {
            if(!isLoggedIn){
                LoginView(isLoggedIn: $isLoggedIn, httpClient: httpClient)
            } else {
                PlantsView(isLoggedIn: $isLoggedIn, httpClient: httpClient)
            }
        }
        .accentColor(.black)
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
