from fastapi import FastAPI, HTTPException, status, Depends
from fastapi.middleware.cors import CORSMiddleware
from fastapi.security import HTTPBearer, HTTPAuthorizationCredentials
from datetime import timedelta
import os
import uuid

from database import supabase
from schemas import UserSignup, UserLogin, Token, UserResponse, ProfilePictureUpdate, StudyPreferencesUpdate
from auth import get_password_hash, verify_password, create_access_token, decode_access_token, ACCESS_TOKEN_EXPIRE_MINUTES

security = HTTPBearer()

def get_current_user(credentials: HTTPAuthorizationCredentials = Depends(security)):
    token = credentials.credentials
    email = decode_access_token(token)
    if not email:
        raise HTTPException(status_code=401, detail="Invalid or expired token")
    
    response = supabase.table("users").select("*").eq("email", email).execute()
    if not response.data:
        raise HTTPException(status_code=401, detail="User not found")
    
    return response.data[0]

app = FastAPI(title="Decagon Backend", description="Authentication API using FastAPI and Supabase")

# Configure CORS so the React frontend can communicate with the backend
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"], # In production, replace with your frontend URL
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/")
def read_root():
    return {"message": "Welcome to the Decagon API"}

@app.post("/signup", response_model=UserResponse, status_code=status.HTTP_201_CREATED)
def signup(user: UserSignup):
    # Check if user already exists
    response = supabase.table("users").select("*").eq("email", user.email).execute()
    if response.data:
        raise HTTPException(status_code=400, detail="Email already registered")

    response_username = supabase.table("users").select("*").eq("username", user.username).execute()
    if response_username.data:
        raise HTTPException(status_code=400, detail="Username already taken")

    # Hash the password
    hashed_password = get_password_hash(user.password)

    # Prepare data
    user_data = {
        "id": str(uuid.uuid4()),
        "username": user.username,
        "email": user.email,
        "hashed_password": hashed_password,
        "is_onboarded": False
    }

    # Insert into Supabase
    try:
        data = supabase.table("users").insert(user_data).execute()
        new_user = data.data[0]
        return UserResponse(**new_user)
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/login", response_model=Token)
def login(user: UserLogin):
    # Find user by email
    response = supabase.table("users").select("*").eq("email", user.email).execute()
    if not response.data:
        raise HTTPException(status_code=400, detail="Incorrect email or password")
    
    db_user = response.data[0]

    # Verify password
    if not verify_password(user.password, db_user["hashed_password"]):
        raise HTTPException(status_code=400, detail="Incorrect email or password")

    # Create access token
    access_token_expires = timedelta(minutes=ACCESS_TOKEN_EXPIRE_MINUTES)
    access_token = create_access_token(
        data={"sub": db_user["email"]}, expires_delta=access_token_expires
    )

    return {"access_token": access_token, "token_type": "bearer"}

@app.post("/onboard/profile-picture", response_model=UserResponse)
def update_profile_picture(data: ProfilePictureUpdate, current_user: dict = Depends(get_current_user)):
    update_data = {
        "profile_picture_url": data.profile_picture_url
    }
    
    try:
        response = supabase.table("users").update(update_data).eq("id", current_user["id"]).execute()
        updated_user = response.data[0]
        return UserResponse(**updated_user)
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/onboard/study-preferences", response_model=UserResponse)
def update_study_preferences(data: StudyPreferencesUpdate, current_user: dict = Depends(get_current_user)):
    # We set is_onboarded to True here, assuming this is the final step of the onboarding flow
    update_data = {
        "study_preferences": data.study_preferences,
        "is_onboarded": True
    }
    
    try:
        response = supabase.table("users").update(update_data).eq("id", current_user["id"]).execute()
        updated_user = response.data[0]
        return UserResponse(**updated_user)
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))
