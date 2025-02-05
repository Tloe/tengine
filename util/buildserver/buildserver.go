package main

import (
	"fmt"
	"os"
	"os/exec"
	"os/signal"
	"strings"
	"syscall"
	"time"
)

func main() {
	ticker := time.NewTicker(5 * time.Second)
	quit := make(chan os.Signal)
	signal.Notify(quit, os.Interrupt, syscall.SIGTERM)
	go func() {
		for {
			select {
			case <-ticker.C:
				if updates, err := checkUpdates(); err != nil {
					fmt.Println("Failed to check for updates,", err)
				} else if updates {
					build()
				}
			case <-quit:
				ticker.Stop()
				return
			}
		}
	}()
	<-quit
	fmt.Println("Stopping tengine build server")
}

func checkUpdates() (updates bool, err error) {
	cmd := exec.Command("git", "pull", "origin", "master")
	out, err := cmd.Output()

	updates = !strings.Contains(string(out), "Already up to date")

	return
}

func build() {

}
