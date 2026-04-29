from pydantic import BaseModel, EmailStr
from typing import List, Optional

class UserSignup(BaseModel):
    username: str
    email: EmailStr
    password: str

class UserLogin(BaseModel):
    email: EmailStr
    password: str

class Token(BaseModel):
    access_token: str
    token_type: str

class UserResponse(BaseModel):
    id: str
    username: str
    email: str
    is_onboarded: bool = False
    profile_picture_url: Optional[str] = None
    study_preferences: Optional[List[str]] = None

class ProfilePictureUpdate(BaseModel):
    profile_picture_url: str

class StudyPreferencesUpdate(BaseModel):
    study_preferences: List[str]
