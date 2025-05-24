package manager

import (
	"bytes"
	"crypto/md5"
	"crypto/rand"
	"fmt"
	"io"
	mathrand "math/rand"
	"os"
	"path/filepath"
	"testing"
	"time"

	"github.com/moevm/grpc_server/internal/worker"
)

func TestIntegrationManagerWorkerSequential(t *testing.T) {
	files := generateTestFiles(t)
	tasksData := createTasksFromFiles(t, files)

	var repeatedTasksData [][]byte
	for _, task := range tasksData {
		for i := 0; i < 3; i++ {
			repeatedTasksData = append(repeatedTasksData, task)
		}
	}
	mathrand.Shuffle(len(repeatedTasksData), func(i, j int) {
		repeatedTasksData[i], repeatedTasksData[j] = repeatedTasksData[j], repeatedTasksData[i]
	})

	go workerInit()
	go worker.Start()
	go taskManager()
	go errorHandler()

	for _, taskBody := range repeatedTasksData {
		task := NewTask(genTaskId(), taskBody)
		tasks.Store(task.id, task)

		start := time.Now()
		for {
			time.Sleep(100 * time.Millisecond)

			val, ok := tasks.Load(task.id)
			if !ok {
				t.Fatalf("Task %d lost", task.id)
			}
			currentTask := val.(*Task)

			switch currentTask.state {
			case taskSolved:
				expectedHash := md5.Sum(taskBody)
				if !bytes.Equal(currentTask.solve, expectedHash[:]) {
					t.Errorf("Task %d: invalid hash", task.id)
				}
				goto NextTask

			default:
				if time.Since(start) > 5*time.Second {
					t.Fatalf("Task %d timeout", task.id)
				}
			}
		}
	NextTask:
	}
}

func generateTestFiles(t *testing.T) []string {
	fileSizes := []int64{
		256,
		512,
		1024,
		2048,
	}

	var filePaths []string

	for i, size := range fileSizes {
		fileName := filepath.Join(t.TempDir(), "test_file_%d.bin")
		filePath := fmt.Sprintf(fileName, i+1)

		err := createTestFile(t, filePath, size)
		if err != nil {
			t.Fatalf("Failed to create test file %s: %v", filePath, err)
		}

		filePaths = append(filePaths, filePath)
		t.Logf("Created test file %s (%d bytes)", filePath, size)
	}

	return filePaths
}

func createTestFile(t *testing.T, filePath string, size int64) error {
	file, err := os.Create(filePath)
	if err != nil {
		return err
	}
	defer file.Close()

	_, err = io.CopyN(file, rand.Reader, size)
	return err
}

func createTasksFromFiles(t *testing.T, filePaths []string) [][]byte {
	var tasks [][]byte
	tasks = append(tasks, make([]byte, 0))

	for _, path := range filePaths {
		data, err := os.ReadFile(path)
		if err != nil {
			t.Fatalf("Failed to read file %s: %v", path, err)
		}
		tasks = append(tasks, data)
	}

	return tasks
}
