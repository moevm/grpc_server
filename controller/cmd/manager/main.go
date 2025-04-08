package main

import (
	"github.com/moevm/grpc_server/internal/manager"
	"github.com/moevm/grpc_server/pkg/binary"
)

func main() {
	binary1Path := "../../binary_trash/trash_1"
	binary2Path := "../../binary_trash/trash_2"
	binary3Path := "../../binary_trash/trash_3"
	tasks := make([][]byte, 3)
	tasks[0] = binary.Read(binary1Path)
	tasks[1] = binary.Read(binary2Path)
	tasks[2] = binary.Read(binary3Path)
	manager.ClusterInit(tasks)
}
