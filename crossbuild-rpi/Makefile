.PHONY: build-docker-arm
build-docker-arm:
	mkdir -p output
	sudo docker build -t aqarahub-armhf-build .
	sudo docker run --rm aqarahub-armhf-build | tar --extract --verbose
