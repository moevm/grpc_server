package manager

import (
	"fmt"
	"github.com/moevm/grpc_server/pkg/converter"
	"log"
	"net"
	"os"
	"os/signal"
	"strings"
	"syscall"
	"time"
)

const (
	TaskCreated          = 3
	TaskRedirection      = 2
	TaskWip              = 1
	TaskSolved           = 0
	WorkerBusy           = 1
	WorkerFree           = 0
	WorkerInitSocketPath = "/tmp/socket/init.sock"
)

type Task struct {
	id    int
	state int
	body  []byte
	solve []byte
}

type Worker struct {
	id       int
	state    int
	taskChan chan *Task
	conn     net.Conn
}

var Workers = make([]*Worker, 0)

var Tasks = make([]*Task, 0)

var TaskIdCounter = 0

var Listener net.Listener

func init() {
	if err := os.RemoveAll(WorkerInitSocketPath); err != nil {
		log.Fatalln(err)
	}
	c := make(chan os.Signal, 10)
	var err error
	Listener, err = net.Listen("unix", WorkerInitSocketPath)
	if err != nil {
		log.Fatalln(err)
	}
	signal.Notify(c, os.Interrupt, syscall.SIGTERM)
	go func() {
		<-c
		Listener.Close()
		os.RemoveAll(WorkerInitSocketPath)
		os.Exit(1)
	}()
}

func GenTaskId() int {
	defer func() {
		TaskIdCounter += 1
	}()
	return TaskIdCounter
}

func NewWorker(workerId int) {
	Workers = append(Workers, &Worker{
		id:       workerId,
		state:    WorkerBusy,
		taskChan: make(chan *Task, 10),
	})
}

func NewTask(taskBody []byte) {
	Tasks = append(Tasks, &Task{
		id:    GenTaskId(),
		state: TaskCreated,
		body:  taskBody,
		solve: make([]byte, 0),
	})
}

func WorkerInit() {
	fmt.Println("Ready for initialization Workers...")
	for {
		conn, err := Listener.Accept()
		if err != nil {
			log.Fatalln()
		}
		workerRawId := make([]byte, 8)
		_, err = conn.Read(workerRawId)
		if err != nil {
			log.Fatalln(err)
		}
		NewWorker(converter.ByteSliceToInt(workerRawId))
		_, err = conn.Write(converter.IntToByteSlice(1))
		if err != nil {
			log.Fatalln(err)
		}
		go Workers[len(Workers)-1].Run()
		fmt.Printf("Worker %v Started\n", Workers[len(Workers)-1].id)
		conn.Close()
	}
}

func (t Task) String() string {
	return fmt.Sprintf("Task id:%v, solution:%v", t.id, t.solve)
}

func (w *Worker) AddTask(task *Task) {
	if w.taskChan == nil || w.state != WorkerFree {
		log.Fatalln("worker is broken")
	}
	if task.body == nil {
		log.Fatalln("task is empty")
	}
	w.state = WorkerBusy
	task.state = TaskWip
	defer func() {
		w.state = WorkerFree
	}()
	var err error
	offset := 0
	taskLen := len(task.body)
	_, err = w.conn.Write(converter.IntToByteSlice(taskLen))
	if err != nil {
		log.Fatalln(err)
	}
	fullFrame := 1024
	for flag := 1; flag != 0; {
		switch {
		case taskLen-offset >= fullFrame:
			_, err = w.conn.Write(converter.IntToByteSlice(fullFrame))
			if err != nil {
				log.Fatalln(err)
			}
			_, err = w.conn.Write(task.body[offset : offset+fullFrame])
			if err != nil {
				log.Fatalln(err)
			}
			offset += fullFrame
		case taskLen-offset < fullFrame:
			frameLen := taskLen - offset
			_, err = w.conn.Write(converter.IntToByteSlice(frameLen))
			if err != nil {
				log.Fatalln(err)
			}
			_, err = w.conn.Write(task.body[offset : offset+frameLen])
			if err != nil {
				log.Fatalln(err)
			}
			offset += frameLen
			flag = 0
		}
	}
	_, err = w.conn.Write(converter.IntToByteSlice(0))
	buf := make([]byte, 8)
	_, err = w.conn.Read(buf)
	if err != nil {
		log.Fatalln(err)
	}
	responseLen := converter.ByteSliceToInt(buf)
	response := make([]byte, responseLen)
	_, err = w.conn.Read(response)
	if err != nil {
		log.Fatalln(err)
	}
	task.solve = append(task.solve, response...)
	task.state = TaskSolved
	fmt.Printf("Task solved: ")
	fmt.Println(task)
}

func (w *Worker) Run() {
	var socketPath strings.Builder
	socketPath.WriteString("/tmp/socket/")
	socketPath.WriteString(fmt.Sprintf("%v.sock", w.id))
	w.state = WorkerFree
	for {
		task := <-w.taskChan
		conn, err := net.Dial("unix", socketPath.String())
		if err != nil {
			log.Fatalln(err)
		}
		w.conn = conn
		fmt.Printf("Task %v accepted by worker %v\n", task.id, w.id)
		w.AddTask(task)
		conn.Close()
	}
}

func LoadBalancer(task *Task) {
	for task.state == TaskCreated {
		for i := range Workers {
			if Workers[i].state == WorkerFree {
				Workers[i].taskChan <- task
				task.state = TaskRedirection
				fmt.Printf("Task %v send to worker %v\n", task.id, Workers[i].id)
				break
			}
		}
	}
}

func TaskManager() {
	for {
		for i := range Tasks {
			if Tasks[i].state == TaskCreated {
				LoadBalancer(Tasks[i])
			}
		}
	}
}

func ClusterInit(rawTaskArray [][]byte) {
	go WorkerInit()
	go TaskManager()
	for i := range rawTaskArray {
		NewTask(rawTaskArray[i])
		time.Sleep(1 * time.Second)
	}
	for {
		time.Sleep(1 * time.Second)
	}
}
