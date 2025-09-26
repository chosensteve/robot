import pygame


keys = pygame.key.get_pressed()
if keys[pygame.K_w]:
    print("W")
elif keys[pygame.K_s]:
    print("S")
elif keys[pygame.K_a]:
    print("A")
elif keys[pygame.K_d]:
    print("D")