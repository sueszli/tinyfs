.PHONY: docker-up # Start Docker containers
docker-up:
	docker compose up -d

.PHONY: docker-build # Build Docker images
docker-build:
	docker compose build

.PHONY: run # Run the server with the default CLI options
run:
	docker compose exec main bazel run //:main

.PHONY: test # Run tests
test:
	docker compose exec main bazel test //:main_test --test_output=all
	docker compose exec main bazel test //:utils_test --test_output=all

.PHONY: debug # Build debug binary and start GDB
debug:
	docker compose exec main bazel build -c dbg //:main
	docker compose exec main gdb bazel-bin/main

.PHONY: release # Build release binary
release:
	docker compose exec main bazel build --config=release //:main

.PHONY: fmt # Format source code
fmt:
	docker compose exec main sh -c "find /workspace \( -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print0 | xargs -0 clang-format -i"

.PHONY: check # Run static analysis and tests for UB and memory leaks
check:
	docker compose exec main sh -c "find /workspace \( -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print0 | xargs -0 cppcheck"
	docker compose exec main bazel test //:main --run_under="valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose"

.PHONY: clean # Stop and remove containers, networks, images and volumes
clean:
	docker compose down --rmi all --volumes --remove-orphans
	docker system prune -a -f

.PHONY: help # generate help message
help:
	@echo "Usage: make [target]\n"
	@grep '^.PHONY: .* #' makefile | sed 's/\.PHONY: \(.*\) # \(.*\)/\1	\2/' | expand -t20
