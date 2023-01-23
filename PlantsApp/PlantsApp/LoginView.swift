//
//  LoginView.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 23/01/2023.
//

import SwiftUI

struct LoginView: View {
    @Binding var isLoggedIn: Bool
    @ObservedObject var httpClient: HttpClient
    @State private var email: String = ""
    @State private var password: String = ""
    @State private var loginError = false
    @State private var loginErrorContent = ""

    var body: some View {
        VStack {
            Form {
                Section("e-mail") {
                    TextField("e-mail", text: $email)
                        .textContentType(.emailAddress)
                        .disableAutocorrection(true)
                }
                Section("hasło") {
                    SecureField("hasło", text: $password)
                        .textContentType(.password)
                        .disableAutocorrection(true)
                }
            }
            
            Button {
                Task {
                    await login()
                }
            } label: {
                Text("Zaloguj się ")
                    .frame(maxWidth: 180)
            }
            .tint(.black)
            .buttonStyle(.borderedProminent)
            .padding(.bottom, 5)
            
            if loginError {
                Section {
                    Text(loginErrorContent)
                        .frame(maxWidth: .infinity, alignment: .center)
                        .foregroundColor(.red)
                }
            }
        }
        .navigationTitle("Logowanie")
        .toolbar {
            NavigationLink("Rejestracja") {
                RegisterView(httpClient: httpClient)
            }
        }
    }
    
    func login() async {
        loginError = false
        if email.isEmpty || password.isEmpty {
            loginError = true
            loginErrorContent = "Wszystkie pola muszą być uzupełnione"
            return
        }
        if !(await httpClient.login(dto: LoginDto(email: email, password: password))) {
            loginError = true
            loginErrorContent = "Niepoprawne dane logowania"
            return
        }
        isLoggedIn = true
    }
}

struct LoginView_Previews: PreviewProvider {
    static var previews: some View {
        LoginView(isLoggedIn: .constant(true), httpClient: HttpClient())
    }
}
