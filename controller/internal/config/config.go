package config

import (
	"log"
	"os"
	"strconv"

	"github.com/joho/godotenv"
)

type ServerConfig struct {
	Host           string
	Port           string
	MaxMessageSize int
	AllowedChars   string
}

func Load() *ServerConfig {
	if err := godotenv.Load(); err != nil {
		log.Printf("No .env file found, using default values")
	}

	return &ServerConfig{
		Host:           getEnv("SERVER_HOST", "0.0.0.0"),
		Port:           getEnv("SERVER_PORT", "50051"),
		MaxMessageSize: getEnvAsInt("MAX_MESSAGE_SIZE", 4*1024*1024), // 4MB
		AllowedChars:   getEnv("ALLOWED_CHARS", "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 !?.,\n"),
	}
}

func getEnv(key, defaultValue string) string {
	if value, exists := os.LookupEnv(key); exists {
		return value
	}
	return defaultValue
}

func getEnvAsInt(key string, defaultValue int) int {
	strValue := getEnv(key, "")
	if strValue == "" {
		return defaultValue
	}
	intValue, err := strconv.Atoi(strValue)
	if err != nil {
		log.Printf("Invalid integer value for %s: %s. Using default: %d", key, strValue, defaultValue)
		return defaultValue
	}
	return intValue
}
