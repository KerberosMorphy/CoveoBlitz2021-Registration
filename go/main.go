package main

import (
	"encoding/json"
	"net/http"
)

type BlitzRequest struct {
	Track []int   `json:"track"`
	Items [][]int `json:"items"`
}

type BlitzResponse []int

func Abs(x int) int {
	if x < 0 {
		return -x
	}
	return x
}

func microChallenge(w http.ResponseWriter, r *http.Request) {
	decoder := json.NewDecoder(r.Body)
	var b BlitzRequest
	decoder.Decode(&b)
	var sums int = 0
	var trackCumsum [1000000]int
	for i, t := range b.Track {
		sums += t
		trackCumsum[i+1] = sums
	}
	var response [1000000]int
	for i, s := range b.Items {
		response[i] = Abs(trackCumsum[s[1]] - trackCumsum[s[0]])
	}
	json.NewEncoder(w).Encode(response[:len(b.Items)])
}

func baseRoute(w http.ResponseWriter, r *http.Request) {}

func handleRequest() {
	http.HandleFunc("/", baseRoute)
	http.HandleFunc("/microchallenge", microChallenge)
	http.ListenAndServe(":27178", nil)
}

func main() {
	handleRequest()
}
