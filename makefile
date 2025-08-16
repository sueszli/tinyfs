.PHONY: docker-up
docker-up:
	docker compose up -d

.PHONY: docker-build
docker-build:
	docker compose build

# .PHONY: run
# run:
# 	@docker exec main g++ -o $(basename $(file)) $(file) -std=c++23
# 	@docker exec main ./$(basename $(file))
# 	@rm -f $(basename $(file))

.PHONY: fmt
fmt:
	docker compose exec main sh -c "find /workspace \( -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print0 | xargs -0 clang-format -i"

.PHONY: lint
lint:
	docker compose exec main sh -c "find /workspace \( -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print0 | xargs -0 clang-tidy"

.PHONY: clean
clean:
	docker compose down --rmi all --volumes --remove-orphans
	docker system prune -a -f
