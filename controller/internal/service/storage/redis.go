package storage

import (
	"context"
	"encoding/json"
	"fmt"
	"time"

	"github.com/redis/go-redis/v9"
)

type Config struct {
	Addr        string
	Password    string
	User        string
	DB          int
	MaxRetries  int
	DialTimeout time.Duration
	Timeout     time.Duration
}

type RedisClient struct {
	db *redis.Client
}

type RequestHash struct {
	Endpoint      string
	CategoriesIds []int
}

func NewRedisClient(ctx context.Context, cfg Config) (*RedisClient, error) {
	db := redis.NewClient(&redis.Options{
		Addr:         cfg.Addr,
		Password:     cfg.Password,
		DB:           cfg.DB,
		Username:     cfg.User,
		MaxRetries:   cfg.MaxRetries,
		DialTimeout:  cfg.DialTimeout,
		ReadTimeout:  cfg.Timeout,
		WriteTimeout: cfg.Timeout,
	})

	if err := db.Ping(ctx).Err(); err != nil {
		fmt.Printf("failed to connect to redis server: %s\n", err.Error())
		return nil, err
	}

	return &RedisClient{db: db}, nil
}

func (c *RedisClient) SaveRequestHash(ctx context.Context, hash RequestHash) error {

	key := fmt.Sprintf("request:hash:%s", hash.Endpoint)

	data, err := json.Marshal(hash.CategoriesIds)

	if err != nil {
		return err
	}

	return c.db.Set(ctx, key, data, 0).Err()
}

func (c *RedisClient) GetRequestHash(ctx context.Context, endpoint string) (*RequestHash, error) {

	key := fmt.Sprintf("request:hash:%s", endpoint)

	cmd := c.db.Get(ctx, key)

	if cmd.Err() != nil {
		return nil, cmd.Err()
	}

	data, err := cmd.Bytes()

	if err != nil {
		return nil, err
	}

	var categoriesID []int

	err = json.Unmarshal(data, &categoriesID)

	if err != nil {
		return nil, err
	}

	return &RequestHash{Endpoint: endpoint, CategoriesIds: categoriesID}, nil
}
