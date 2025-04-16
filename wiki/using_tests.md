## Instructions for using tests

- download all dependencies of ```go mod download``` or add a separate testify ```go get github.com/stretchr/testify```. If you have any problems, you can use ```go mod tidy``` to synchronize dependencies.

- you need to run the tests from the root of the project ```go test -v ./internal/grpcserver```. Also can run only unit tests ```go test -v -run ^TestUploadFile_ ./internal/grpcserver``` or specific test ```go test -v -run TestUploadFile_TextValidation ./internal/grpcserver```