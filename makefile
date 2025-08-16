.PHONY: docker-up
docker-up:
	docker compose up -d

.PHONY: docker-build
docker-build:
	docker compose build

.PHONY: run
run:
	docker compose exec main bazel build //:tinyfs
	docker compose exec main bazel run //:tinyfs

.PHONY: debug
debug:
	docker compose exec main bazel build -c dbg //:tinyfs
	docker compose exec main gdb bazel-bin/tinyfs

.PHONY: release
release:
	docker compose exec main bazel build --config=release //:tinyfs

.PHONY: fmt
fmt:
	docker compose exec main sh -c "find /workspace \( -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print0 | xargs -0 clang-format -i"

.PHONY: check
check:
	docker compose exec main sh -c "find /workspace \( -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print0 | xargs -0 cppcheck"
	docker compose exec main bazel run //:tinyfs --run_under="valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose"

.PHONY: clean
clean:
	docker compose down --rmi all --volumes --remove-orphans
	docker system prune -a -f
