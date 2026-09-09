import secrets
import random
from typing import Annotated
from fastapi import FastAPI, Response, Cookie
from pydantic import BaseModel
from fastapi.responses import PlainTextResponse, JSONResponse
from sqlmodel import Field, SQLModel, create_engine, Session, select
import tomllib  # apparently, toml is included in python now
import json

app = FastAPI()
dmkey = secrets.token_urlsafe(12)
playerkeys = [
    "UNSAFEKEY",
    secrets.token_urlsafe(12),
    secrets.token_urlsafe(12),
    secrets.token_urlsafe(12),
    secrets.token_urlsafe(12),
    secrets.token_urlsafe(12),
    secrets.token_urlsafe(12),
    secrets.token_urlsafe(12),
    secrets.token_urlsafe(12),
    secrets.token_urlsafe(12),
]
# THERE HAS GOT TO BE A BETTER WAY TO DO THIS !!


class idlookup(SQLModel, table=True):
    # standard item id types
    # single digit: players
    # double digit: ground types
    # 1** : npcs
    # 2** : enemies
    # 3** : landmarks
    # 4** : items
    # 5**~9** : user defined
    id: int | None = Field(default=None, primary_key=True)
    lookupid: int
    name: str
    stats: str


engine = create_engine("sqlite:///world.db", echo=True)
SQLModel.metadata.create_all(engine)

with open("config.toml", "rb") as f:
    config = tomllib.load(f)


class authenticateconf(BaseModel):
    key: str  # the key(s) the client(s) will send
    typ: int  # one of   1: DM_KEY   2: [playerkeys]


class sendmapping(BaseModel):
    mappng: list


class sendobjdef(BaseModel):
    names: str
    status: str

class sendmsg(BaseModel):
        playerID: int
        undername: str
        content: str
        


mapping = {"map":[
    [0, 1, 2, 3, 4],
    [1, 2, 3, 4, 5],
    [2, 3, 4, 5, 6],
    [3, 4, 5, 6, 7],
    [4, 5, 6, 7, 8],
]}



async def objlookupfunc(lookup: int):
    with Session(engine) as session:
        statement = select(idlookup).where(idlookup.lookupid == lookup)
        results = session.exec(statement)
        return results.first().model_dump()


async def objcreationfunc(
    making: int,
    nam: str,
    state: str,
):
    with Session(engine) as session:
        session.add(idlookup(lookupid=making, name=nam, stats=state))
        session.commit()


@app.get("/")
async def connect():
    return JSONResponse({"hello": "world"})


@app.get("/info/{objid}")
async def objlookup(objid):
    objectstepone = await objlookupfunc(objid)
    return PlainTextResponse(
        f'name="{objectstepone["name"]}"\n{objectstepone["stats"].replace("\\n", "\n")}'
    )


@app.post("/make/{objid}")
async def objmake(
    objid, objeclass: sendobjdef, AUTH: Annotated[str | None, Cookie()] = None
):
    """create an object, simple enough"""
    if AUTH == dmkey:
        await objcreationfunc(objid, objeclass.names, objeclass.status)
        return PlainTextResponse("succesfully created!")


@app.get("/coffee")
async def teapot():
    """this is in entirely as a joke, clients can do whatever with it"""
    return PlainTextResponse(status_code=418) # i have a bit of a joke of adding 418 response codes to web related things i make, we cant brew coffee!



@app.get("/map")
async def mapget():
    return JSONResponse(mapping)


@app.post("/map")
async def mapset(mappings: sendmapping, AUTH: Annotated[str | None, Cookie()] = None):
    """POST an array to this to set the map that will be sent by GET requests to /map!"""
    if AUTH == dmkey:
        global mapping
        mapping.update({"map":mappings.mappng})
        print(mapping)
        return PlainTextResponse(str(mapping))




@app.post("/authenticate")
async def authenticatething(authenticator: authenticateconf):
    """client will send a POST request containing what type of user it wants to authenticate as AS WELL as the (configured on server) key for said user\n
    why not just use Oauth?  cause i dont understand how to and im kindof scared of it"""
    if authenticator.typ == 1:
        if authenticator.key == config["DM_KEY"]:
            content = 0
            response = JSONResponse(content=content)
            response.set_cookie(
                key="AUTH", value=dmkey, max_age=config["MAX_AGE"]
            )  # authentication stuff lasts for exactly 4 hours before you have to set a new one
            return response
        else:
            return PlainTextResponse("FAILED AUTHENTICATION", status_code=401)
    if authenticator.typ == 2:
        try:
            if config["playerkeys"][authenticator.key] in range(10):
                content = config["playerkeys"][authenticator.key]
                response = JSONResponse(content=content)
                response.set_cookie(
                    key="AUTH",
                    max_age=config["MAX_AGE"],
                    value=playerkeys[config["playerkeys"][authenticator.key]],
                )
                return response
            else:
                return PlainTextResponse(
                    "FAILED AUTHENTICATION", status_code=401
                )  # how are you going to even reach this??
        except:
            return PlainTextResponse(
                "internal server error, its likely you used the wrong key!",
                status_code=401,
            )
@app.get("/roll/{xdX}")        
async def rolling(xdX):
   """rolls a dice as specified in the url"""
   dtype = range(1, int(xdX.split("d")[1]) + 1)
   resultt = 0
   for i in range(1, int(xdX.split("d")[0]) + 1):
        resultt += random.choice(dtype)
   with open("MESSAGELOG.txt", "a") as f:
        f.write(f"Rolled {xdX} and got a {resultt}\n")     
   return PlainTextResponse(str(resultt))

@app.post("/msg")
async def message(messagedata: sendmsg ,AUTH: Annotated[str | None, Cookie()] = None):
    """appends a message to the message log"""
    if messagedata.playerID == 0:
        if AUTH == dmkey:
            with open("MESSAGELOG.txt", "a") as f:
                f.write(messagedata.undername + ": " + messagedata.content.replace("_", " ") + "\n")
        else:
            return PlainTextResponse("unauthenticated users cannot send messages", status_code=401)    
    else:
        if AUTH == playerkeys[messagedata.playerID]:
            objectsteponetwo = await objlookupfunc(messagedata.playerID)
            with open("MESSAGELOG.txt", "a") as f:
                f.write(objectsteponetwo["name"] + ": " + messagedata.content.replace("_", " ") + "\n")
        else:
            return PlainTextResponse("unauthenticated users cannot send messages", status_code=401)            
@app.get("/msg")
async def getmsg():
    """gets the entire message log from MESSAGELOG.txt"""
    content = "messages"
    with open("MESSAGELOG.txt", "r") as f:
        content = f.read()
    return PlainTextResponse(content)
@app.get("/msg/clear")
async def clrmsg(AUTH: Annotated[str | None, Cookie()] = None):
    """clears MESSAGELOG. be careful!"""
    if AUTH == dmkey:
        with open("MESSAGELOG.txt", "w") as f:
            return PlainTextResponse("messagelog cleared!")
    else:
        return PlainTextResponse("people who arent the dm cannot clear the messagelog", status_code=401)        




