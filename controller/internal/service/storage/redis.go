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
	TTLSuccess         time.Duration
	TTLUnknown         time.Duration
}

type RedisClient struct {
	db *redis.Client
	TTLSuccess         time.Duration
	TTLUnknown         time.Duration
}

type RequestCache struct {
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

	return &RedisClient{db: db, TTLSuccess: cfg.TTLSuccess, TTLUnknown: cfg.TTLUnknown}, nil
}

func (c *RedisClient) SaveRequestCache(ctx context.Context, cache RequestCache, isUnknown bool) error {

	key := fmt.Sprintf("request:cache:%s", cache.Endpoint)

	data, err := json.Marshal(cache.CategoriesIds)

	if err != nil {
		return err
	}

	ttl := c.TTLSuccess
	if isUnknown {
		ttl = c.TTLUnknown
	}

	return c.db.Set(ctx, key, data, ttl).Err()
}

func (c *RedisClient) GetRequestCache(ctx context.Context, endpoint string) (*RequestCache, error) {

	key := fmt.Sprintf("request:cache:%s", endpoint)

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

	return &RequestCache{Endpoint: endpoint, CategoriesIds: categoriesID}, nil
}
