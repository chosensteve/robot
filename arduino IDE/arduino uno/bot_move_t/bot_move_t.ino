int s0 = 14;
int s1 = 27;
int s2 = 26;
int s3 = 25;
int sigPin = 34;

int AIN1 = 16;
int AIN2 = 17;
int PWMA = 18;
int BIN1 = 19;
int BIN2 = 21;
int PWMB = 22;
int STBY = 5;

float Kp = 30;
float Ki = 0;
float Kd = 0.2;
int base = 700;
float sum_err = 0;
float last_err = 0;
int turn_t = 300;

int read_mux(int ch) {
  digitalWrite(s0, ch & 1);
  digitalWrite(s1, (ch >> 1) & 1);
  digitalWrite(s2, (ch >> 2) & 1);
  digitalWrite(s3, (ch >> 3) & 1);
  delayMicroseconds(50);
  return analogRead(sigPin);
}

float read_line(int lvl[]) {
  int v[6];
  for (int i = 0; i < 6; i++) v[i] = read_mux(i);
  for (int i = 0; i < 6; i++) lvl[i] = (v[i] < 2000) ? 1 : 0;

  float t = 0, c = 0;
  for (int i = 0; i < 6; i++) {
    t += i * lvl[i];
    c += lvl[i];
  }
  if (c == 0) return 2.5;
  return t / c;
}

void motor(int l, int r) {
  l = constrain(l, -255, 255);
  r = constrain(r, -255, 255);

  if (l >= 0) {
    digitalWrite(AIN1, 0); digitalWrite(AIN2, 1); analogWrite(PWMA, l);
  } else {
    digitalWrite(AIN1, 1); digitalWrite(AIN2, 0); analogWrite(PWMA, -l);
  }

  if (r >= 0) {
    digitalWrite(BIN1, 0); digitalWrite(BIN2, 1); analogWrite(PWMB, r);
  } else {
    digitalWrite(BIN1, 1); digitalWrite(BIN2, 0); analogWrite(PWMB, -r);
  }
}

void setup() {
  pinMode(s0, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  pinMode(s3, OUTPUT);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(STBY, OUTPUT);

  digitalWrite(STBY, 1);
}

void loop() {
  int lvl[6];
  float pos = read_line(lvl);
  float err = 2.5 - pos;
  sum_err += err;

  if (lvl[0] == 1 && (lvl[1] + lvl[2] + lvl[3] + lvl[4] + lvl[5] == 0)) {
    motor(255, -255);
    delay(turn_t);
    return;
  }

  if (lvl[5] == 1 && (lvl[0] + lvl[1] + lvl[2] + lvl[3] + lvl[4] == 0)) {
    motor(-255, 255);
    delay(turn_t);
    return;
  }

  float corr = (Kp * err) + (Ki * sum_err) + (Kd * (err - last_err));
  int left = base + (corr * 2);
  int right = base - (corr * 2);

  motor(left, right);
  last_err = err;
  delayMicroseconds(5000);
}
