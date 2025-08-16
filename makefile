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

.PHONY: clean
clean:
	docker compose down --rmi all --volumes --remove-orphans
	docker system prune -a -f
