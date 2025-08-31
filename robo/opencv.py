import cv2

img123243 = cv2.imread(r"C:\Users\Dev\Desktop\cs\robo\resources\red_cyl.png")
cap = cv2.VideoCapture(2)

while True:
    success, img = cap.read()
    imggray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    cv2.imshow("video", imggray)
    if cv2.waitKey(1) and 0xFF ==ord('q'):
        break