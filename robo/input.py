import pygame

pygame.init()
screen = pygame.display.set_mode((1, 1))  # small window needed

running = True
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    keys = pygame.key.get_pressed()

    if keys[pygame.K_w]:
        print("Forward")
    elif keys[pygame.K_s]:
        print("Backward")
    elif keys[pygame.K_a]:
        print("Left")
    elif keys[pygame.K_d]:
        print("Right")
    elif keys[pygame.K_q]:
        running = False

pygame.quit()