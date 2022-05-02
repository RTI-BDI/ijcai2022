using System.Collections;
using System.Collections.Generic;
using System;
using UnityEngine;
using Newtonsoft.Json.Linq;
using System.IO;
using Newtonsoft.Json;

public class GameManager : MonoBehaviour
{
    [SerializeField]
    private GridManager grid;
	private static float tileSize;
	private static Vector2 gridPosition;
	[SerializeField]
	private Parser setParse;
    private static Parser parser;

    private static Dictionary<string, int> constants = new Dictionary<string, int>();

    private List<GameObject> collectors = new List<GameObject>();
    private List<GameObject> producers = new List<GameObject>();
    private List<GameObject> woods = new List<GameObject>();
    private List<GameObject> stones = new List<GameObject>();
    private List<GameObject> storages = new List<GameObject>();
    private List<GameObject> rechargeStations = new List<GameObject>();

	private static List<KeyValuePair<string, string>> updates = new List<KeyValuePair<string, string>>();
    
    private Client client;
	private static int coins = 0;

	private enum State
	{
		Playing,
		Pause,
		Waiting,
		Planning
	}

	private static int frame;
	private static State state;
	private int planNumber = 1;

	// Start is called before the first frame update
	void Start()
    {
        Application.targetFrameRate = 60;

		//Initialize the game
		parser = setParse;
		parser.Parse();
		constants = new Dictionary<string, int>();

		InstantiateGame();

		//Reset static variables
		frame = 0;
		GoPause();

		//Visual Instantiation of grid and objects
		grid.SetGridSize(constants["grid-size"]);
		grid.GenerateGrid();
		tileSize = grid.GetTileSize();
		gridPosition = grid.GetPostion();

		PositionEntities();

		client = new Client();
		client.Connect();

		KronosimInitialization(client);

	}

	// Update is called once per frame
	void Update()
    {

		switch (state)
		{
			case State.Playing:
				frame++;
				UIManager.UpdateFrameText(frame);
				KronosimInteraction();

				if(frame % 6 == 0)
				{
					coins++;
					UIManager.UpdateCoinText(coins);
				}

				break;
			case State.Pause:
				UIManager.UpdateFrameText(frame);
				break;
			default:
				break;
		}

		

        if(Input.GetKeyDown("g"))
            collectors[0].GetComponent<Collector>().MoveUp();
    }
    
    private void InstantiateGame()
    {
        //References
        GameObject referenceCollector = (GameObject)Instantiate(Resources.Load("Collector"));
        GameObject referenceProducer = (GameObject)Instantiate(Resources.Load("Producer"));
        GameObject referenceWood = (GameObject)Instantiate(Resources.Load("Wood"));
        GameObject referenceStone = (GameObject)Instantiate(Resources.Load("Stone"));
        GameObject referenceStorage = (GameObject)Instantiate(Resources.Load("Storage"));
        GameObject referenceRechargeStation = (GameObject)Instantiate(Resources.Load("RechargeStation"));

        //Instantiate GameObjects
        foreach (Parameter obj in parser.GetProblem().objects)
        {
            switch (obj.type)
            {
                case "collector":        
                    GameObject collector = (GameObject)Instantiate(referenceCollector);
                    collector.GetComponent<Collector>().SetName(obj.name);
                    collectors.Add(collector);
                    break;
                case "producer":
                    GameObject producer = (GameObject)Instantiate(referenceProducer);
                    producer.GetComponent<Producer>().SetName(obj.name);
                    producers.Add(producer);
                    break;
                case "wood":
                    GameObject wood = (GameObject)Instantiate(referenceWood);
                    wood.GetComponent<Wood>().SetName(obj.name);
                    woods.Add(wood);
                    break;
                case "stone":
                    GameObject stone = (GameObject)Instantiate(referenceStone);
                    stone.GetComponent<Stone>().SetName(obj.name);
                    stones.Add(stone);
                    break;
                case "storage":
                    GameObject storage = (GameObject)Instantiate(referenceStorage);
                    storage.GetComponent<Storage>().SetName(obj.name);
                    storages.Add(storage);
                    break;
                case "r_station":
                    GameObject rechargeStation = (GameObject)Instantiate(referenceRechargeStation);
                    rechargeStation.GetComponent<RechargeStation>().SetName(obj.name);
                    rechargeStations.Add(rechargeStation);
                    break;
                default:
                    break;
            }
        }

        //Fetch attributes initializations from problem
        foreach (Expression init in parser.GetProblem().initializations)
        {
            //Not  considering constant beliefs
            if(init.exp_1.node.belief.type == Belief.BeliefType.Predicate)
            {
                //This is a predicate !!! Not yet an usage
            } else if (init.exp_1.node.belief.type == Belief.BeliefType.Function)
            {
                string type;
                type = parser.GetProblem().objects.Find(o => o.name == init.exp_1.node.belief.param[0].name).type;

                switch (type)
                {
                    case "collector":
                        GameObject collector = null;
                        foreach (GameObject g in collectors)
                        {
                            if(g.GetComponent<Collector>().GetName() == init.exp_1.node.belief.param[0].name)
                            {
                                collector = g;
                            }
                        }
                        if (collector != null)
                            collector.GetComponent<Collector>().SetAttribute(init.exp_1.node.belief.name, init.exp_2.node.value);
                        break;
                    case "producer":
                        GameObject producer = null;
                        foreach (GameObject g in producers)
                        {
                            if(g.GetComponent<Producer>().GetName() == init.exp_1.node.belief.param[0].name)
                            {
                                producer = g;
                            }
                        }
                        if (producer != null)
                            producer.GetComponent<Producer>().SetAttribute(init.exp_1.node.belief.name, init.exp_2.node.value);
                        break;
                    case "wood":
                        GameObject wood = null;
                        foreach (GameObject g in woods)
                        {
                            if (g.GetComponent<Wood>().GetName() == init.exp_1.node.belief.param[0].name)
                            {
                                wood = g;
                            }
                        }
                        if (wood != null)
                            wood.GetComponent<Wood>().SetAttribute(init.exp_1.node.belief.name, init.exp_2.node.value);
                        break;
                    case "stone":
                        GameObject stone = null;
                        foreach (GameObject g in stones)
                        {
                            if (g.GetComponent<Stone>().GetName() == init.exp_1.node.belief.param[0].name)
                            {
                                stone = g;
                            }
                        }
                        if(stone != null)
                            stone.GetComponent<Stone>().SetAttribute(init.exp_1.node.belief.name, init.exp_2.node.value);
                        break;
                    case "storage":
                        GameObject storage = null;
                        foreach (GameObject g in storages)
                        {
                            if (g.GetComponent<Storage>().GetName() == init.exp_1.node.belief.param[0].name)
                            {
                                storage = g;
                            }
                        }
                        if (storage != null)
                            storage.GetComponent<Storage>().SetAttribute(init.exp_1.node.belief.name, init.exp_2.node.value);
                        break;
                    case "r_station":
                        GameObject rechargeStation = null;
                        foreach (GameObject g in rechargeStations)
                        {
                            if (g.GetComponent<RechargeStation>().GetName() == init.exp_1.node.belief.param[0].name)
                            {
                                rechargeStation = g;
                            }
                        }
                        if (rechargeStation != null)
                            rechargeStation.GetComponent<RechargeStation>().SetAttribute(init.exp_1.node.belief.name, init.exp_2.node.value);
                        break;
                    default:
                        break;
                }
            } else
            {
                //Enter this scope if the expression represents a constant
                constants.Add(init.exp_1.node.belief.name, int.Parse(init.exp_2.node.value));
            }
        }

        Destroy(referenceCollector);
        Destroy(referenceProducer);
        Destroy(referenceWood);
        Destroy(referenceStone);
        Destroy(referenceStorage);
        Destroy(referenceRechargeStation);
    }

	private void PositionEntities()
	{
		//Move the controllers to their location
		foreach (GameObject g in collectors)
		{
			g.GetComponent<Collector>().MoveToDestination(grid.GetTileSize(), grid.GetPostion());
		}
		foreach (GameObject g in producers)
		{
			g.GetComponent<Producer>().MoveToDestination(grid.GetTileSize(), grid.GetPostion());
		}
		foreach (GameObject g in woods)
		{
			g.GetComponent<Wood>().MoveToDestination(grid.GetTileSize(), grid.GetPostion());
		}
		foreach (GameObject g in stones)
		{
			g.GetComponent<Stone>().MoveToDestination(grid.GetTileSize(), grid.GetPostion());
		}
		foreach (GameObject g in storages)
		{
			g.GetComponent<Storage>().MoveToDestination(grid.GetTileSize(), grid.GetPostion());
		}
		foreach (GameObject g in rechargeStations)
		{
			g.GetComponent<RechargeStation>().MoveToDestination(grid.GetTileSize(), grid.GetPostion());
		}
	}

	public static int GetBatteryDecrease(string actionName)
	{
		Action a = parser.retrieveAction(actionName);
		foreach (Expression pc in a.postConditions)
		{
			if(pc.exp_1.node.value == "decrease" && pc.exp_1.exp_1.node.belief.name == "battery-amount")
			{
				return int.Parse(pc.exp_1.exp_2.node.value);
			}
		}

		return 0;
	}

	public static float GetTileSize()
	{
		return tileSize;
	}

	public static Vector2 GetGridPosition()
	{
		return gridPosition;
	}

	public static int GetFrame()
	{
		return frame;
	}

	public static Dictionary<string, int> GetConstants()
	{
		return constants;
	}

	public static int GetCoins()
	{
		return coins;

	}

	public static void DescreaseCoins(int decrease)
	{
		coins = coins - decrease;
		UIManager.UpdateCoinText(coins);
	}

	public void GoPause()
	{
		state = State.Pause;
		UIManager.InPause();

		foreach (GameObject c in collectors)
		{
			c.GetComponent<Collector>().Pause();
		}
		foreach (GameObject p in producers)
		{
			p.GetComponent<Producer>().Pause();
		}
		foreach (GameObject s in storages)
		{
			s.GetComponent<Storage>().Pause();
		}
	}

	public static void GoTemporaryPause()
	{
		state = State.Pause;
	}

	public void GoPlay()
	{
		state = State.Playing;
		UIManager.InPlay();

		foreach(GameObject c in collectors)
		{
			c.GetComponent<Collector>().Resume();
		}
		foreach (GameObject p in producers)
		{
			p.GetComponent<Producer>().Resume();
		}
		foreach (GameObject s in storages)
		{
			s.GetComponent<Storage>().Resume();
		}
	}

	public static void Resume()
	{
		state = State.Playing;
	}

	private string ExtractAction(string groundedAction)
	{
		string result = string.Empty;

		bool stop = false;
		foreach (Char c in groundedAction)
		{
			if (!stop)
			{
				if(c == '_')
				{
					stop = true;
				} else
				{
					result = result + c;
				}
			}
		}
		return parser.MapAction(result);
	}

	private List<string> ExtractEntities(string groundedAction)
	{
		bool addingEntity = false;

		List<string> result = new List<string>();
		string tempResult = "";

		int counter = 0;

		foreach (Char c in groundedAction)
		{
			if(c == '_')
			{
				if (addingEntity)
				{
					result.Add(tempResult);
				}

				addingEntity = true;
				tempResult = string.Empty;
			} else
			{
				if (addingEntity)
				{
					tempResult = tempResult + c;
				}
			}

			if(counter == groundedAction.Length - 1)
			{
				result.Add(tempResult);

			}

			counter++;
		}

		return result;
	}

	private KeyValuePair<GameObject, string> SearchEntity(string name)
	{
		foreach (GameObject c in collectors)
		{
			if(c.GetComponent<Collector>().GetName().Equals(name))
			{
				return new KeyValuePair<GameObject, string>(c, "collector");
			}
		}

		foreach (GameObject p in producers)
		{
			if (p.GetComponent<Producer>().GetName().Equals(name))
			{
				return new KeyValuePair<GameObject, string>(p, "producer");
			}
		}

		foreach (GameObject s in storages)
		{
			if (s.GetComponent<Storage>().GetName().Equals(name))
			{
				return new KeyValuePair<GameObject, string>(s, "storage");
			}
		}

		foreach (GameObject w in woods)
		{
			if (w.GetComponent<Wood>().GetName().Equals(name))
			{
				return new KeyValuePair<GameObject, string>(w, "wood");
			}
		}
		
		foreach (GameObject s in stones)
		{
			if (s.GetComponent<Stone>().GetName().Equals(name))
			{
				return new KeyValuePair<GameObject, string>(s, "stone");
			}
		}
		
		foreach (GameObject r in rechargeStations)
		{
			if (r.GetComponent<RechargeStation>().GetName().Equals(name))
			{
				return new KeyValuePair<GameObject, string>(r, "r_station");
			}
		}

		return new KeyValuePair<GameObject, string>(null, "");
	}

	private void ExecuteAction(string action, GameObject entity)
	{
		entity.SendMessage(action);
	}

	private void StopAction(string action, KeyValuePair<GameObject, string> entity)
	{
		switch (entity.Value)
		{
			case "collector":
				entity.Key.GetComponent<Collector>().StopAction(action);
				break;
			case "producer":
				entity.Key.GetComponent<Producer>().StopAction(action);
				break;
			case "storage":
				entity.Key.GetComponent<Storage>().StopAction(action);
				break;
			default:
				break;
		}
	}

	public static void addUpdate(string fileName, string content)
	{
		updates.Add(new KeyValuePair<string, string>(fileName, content));
	}


	private void KronosimInteraction()
	{
		//If game going on...
		if(state == State.Playing)
		{

			//First, send all the updates to kronosim
			foreach (KeyValuePair<string, string> kvp in updates)
			{
				Debug.Log("Sending Update: " + kvp.Key + "/" + kvp.Value);
				string updateResponse = client.SendUpdate(kvp.Key, kvp.Value);
				JObject jsonUpdateResponse = JObject.Parse(updateResponse);
				if(jsonUpdateResponse["status"].ToString().Equals("success"))
				{
					Debug.Log("Update OK : " + jsonUpdateResponse["file"].ToString());
				} else
				{
					Debug.Log("Update ERROR : " + jsonUpdateResponse["status"].ToString());
				}
			}
			
			//Clear the updates
			updates = new List<KeyValuePair<string, string>>();

			//Move the simulation forward
			string runResponse = client.SendRun(frame);
			JObject jsonRunResponse = JObject.Parse(runResponse);
			if (jsonRunResponse["command"].ToString().Equals("ACK_RUN"))
			{
				JArray actions = jsonRunResponse["new_actions"] as JArray;
				foreach (JToken token in actions)
				{
					string action = ExtractAction(token.ToString());
					List<string> entities = ExtractEntities(token.ToString());
					foreach (string entity in entities)
					{
						ExecuteAction(action, SearchEntity(entity).Key);
					}
				}
			} else if (jsonRunResponse["command"].ToString().Equals("ERR_RUN"))
			{
				Debug.Log("Plan failed : " + jsonRunResponse["reason"].ToString());
				state = State.Waiting;
			} else if (jsonRunResponse["command"].ToString().Equals("NEW_SOLUTION"))
			{
				Debug.Log("Kronosim found a new solution: ");

				//Check if plan is finished
				JArray completedGoals = jsonRunResponse["completed_goals"] as JArray;
				foreach (JToken token in completedGoals)
				{
					if(token.ToString() == "plan_execution")
					{
						Debug.Log("WIN");
					}
				}

				//Stop all the actions
				JArray actionsToStop = jsonRunResponse["stopped_actions"] as JArray;
				foreach (JToken token in actionsToStop)
				{
					
					if(token.ToString() != "plan_execution") {
						string action = ExtractAction(token.ToString());
						List<string> entitiesInvolved = ExtractEntities(token.ToString());
						foreach (string e in entitiesInvolved)
						{
							StopAction(action, SearchEntity(e));
						}
					}
				}
				//Start new actions
				JArray actionsToStart = jsonRunResponse["new_actions"] as JArray;
				foreach (JToken token in actionsToStart)
				{
					if(token.ToString() != "plan_execution") {
						Debug.Log(token.ToString());
						string action = ExtractAction(token.ToString());
						List<string> entitiesInvolved = ExtractEntities(token.ToString());
						foreach (string e in entitiesInvolved)
						{
							ExecuteAction(action, SearchEntity(e).Key);
						}
					}
				}
			
			} else if (jsonRunResponse["command"].ToString().Equals("NO_SOLUTION"))
			{
			
				//Stop all the coroutines and bring back any moving robot
				foreach (GameObject g in collectors)
				{
					g.GetComponent<Collector>().StopAllActions();			
				}
				foreach (GameObject g in producers)
				{
					g.GetComponent<Producer>().StopAllActions();
				}
				foreach (GameObject g in storages)
				{
					g.GetComponent<Storage>().StopAllActions();
				}		
				
				//Derive a new problem based on what is happening and write it in the problem config. file
				Snapshot();

				//Parse the new problem
				parser.ParseProblem();

				//Call the planner to make a new plan
				parser.CallPlanner();

				//Parse the plan
				parser.ParsePlan();

				//Send new Plan to Kronosim
				string updateResponse = client.SendUpdate("planset.json", File.ReadAllText("./Assets/kronosim/inputs/planset.json"));
				JObject jsonUpdateResponse = JObject.Parse(updateResponse);
				if (jsonUpdateResponse["status"].ToString().Equals("success"))
				{
					Debug.Log("Update OK : " + jsonUpdateResponse["file"].ToString());
				}
				else
				{
					Debug.Log("Update ERROR : " + jsonUpdateResponse["status"].ToString());
				}
				
				//Generate new desireset
				parser.GenerateDesireSet();

				//Send new Desireset to Kronosim
				updateResponse = client.SendUpdate("desireset.json", File.ReadAllText("./Assets/kronosim/inputs/desireset.json"));
				jsonUpdateResponse = JObject.Parse(updateResponse);
				if (jsonUpdateResponse["status"].ToString().Equals("success"))
				{
					Debug.Log("Update OK : " + jsonUpdateResponse["file"].ToString());
				}
				else
				{
					Debug.Log("Update ERROR : " + jsonUpdateResponse["status"].ToString());
				}

			}
		} 
	}

	private void KronosimInitialization(Client c)
	{
		string response = c.SendInitialize("beliefset.json", File.ReadAllText("./Assets/kronosim/inputs/beliefset.json"));
		JObject jsonResponse = JObject.Parse(response);
		Debug.Log(jsonResponse["command"].ToString() + " -- Initialized " + jsonResponse["file"].ToString());
		
		response = c.SendInitialize("skillset.json", File.ReadAllText("./Assets/kronosim/inputs/skillset.json"));
		jsonResponse = JObject.Parse(response);
		Debug.Log(jsonResponse["command"].ToString() + " -- Initialized " + jsonResponse["file"].ToString());

		response = c.SendInitialize("desireset.json", File.ReadAllText("./Assets/kronosim/inputs/desireset.json"));
		jsonResponse = JObject.Parse(response);
		Debug.Log(jsonResponse["command"].ToString() + " -- Initialized " + jsonResponse["file"].ToString());
		
		response = c.SendInitialize("servers.json", File.ReadAllText("./Assets/kronosim/inputs/servers.json"));
		jsonResponse = JObject.Parse(response);
		Debug.Log(jsonResponse["command"].ToString() + " -- Initialized " + jsonResponse["file"].ToString());

		response = c.SendInitialize("planset.json", File.ReadAllText("./Assets/kronosim/inputs/planset.json"));
		jsonResponse = JObject.Parse(response);
		Debug.Log(jsonResponse["command"].ToString() + " -- Initialized " + jsonResponse["file"].ToString());

		response = c.SendInitialize("sensors.json", "{ \"0\" : [ ] }");
		jsonResponse = JObject.Parse(response);
		Debug.Log(jsonResponse["command"].ToString() + " -- Initialized " + jsonResponse["file"].ToString());

		response = c.SendInitialize("update-beliefset.json", File.ReadAllText("./Assets/kronosim/inputs/update-beliefset.json"));
		jsonResponse = JObject.Parse(response);
		Debug.Log(jsonResponse["command"].ToString() + " -- Initialized " + jsonResponse["file"].ToString());
		
		response = c.SendSetupCompleted();
		Debug.Log(response);
	}

	public void KronosimExit()
	{
		string response = client.SendExit();
		Debug.Log(response);
	}

	public void UpdateBattery()
	{
		KeyValuePair<GameObject, string> obj = SearchEntity(UIManager.inspectedObj);
		UIManager.UpdateBattery(obj, coins);
	}

	private void Snapshot()
	{
		//keep goal and problem attributes
		string pName = "";
		string activeDomain = "";
		string goal = "";

		string jsonProblem = File.ReadAllText("./Assets/JSON/Problem.json");
		JObject objectProblem = JObject.Parse(jsonProblem);
		JArray problem = objectProblem["problem"] as JArray;

		pName = problem.First.Next.ToString();
		activeDomain = problem.First.Next.Next.ToString();

		foreach (JToken token in problem)
		{
			if(token is JArray && token.First.ToString() == "defineGoal")
			{
				goal = token.ToString(); 
			}
		}

		//extract from situation the new problem

		string jsonStr = "";
		jsonStr = jsonStr + "{ \"problem\" : [ \"defineProblem\", \"" + pName + "\", \"" + activeDomain + "\", ";
		jsonStr = jsonStr + " [\"defineObjects\", [\"array\", ";

		//defining objects
		foreach (GameObject c in collectors)
		{
			jsonStr = jsonStr + " [ \"parameter\", \"" + c.GetComponent<Collector>().GetName() + "\", \"collector\" ],";
		}

		foreach (GameObject p in producers)
		{
			jsonStr = jsonStr + " [ \"parameter\", \"" + p.GetComponent<Producer>().GetName() + "\", \"producer\" ],";
		}

		foreach (GameObject w in woods)
		{
			jsonStr = jsonStr + " [ \"parameter\", \"" + w.GetComponent<Wood>().GetName() + "\", \"wood\" ],";
		}

		foreach (GameObject s in stones)
		{
			jsonStr = jsonStr + " [ \"parameter\", \"" + s.GetComponent<Stone>().GetName() + "\", \"stone\" ],";
		}

		foreach (GameObject s in storages)
		{
			jsonStr = jsonStr + " [ \"parameter\", \"" + s.GetComponent<Storage>().GetName() + "\", \"storage\" ],";
		}

		foreach (GameObject r in rechargeStations)
		{
			jsonStr = jsonStr + " [ \"parameter\", \"" + r.GetComponent<RechargeStation>().GetName() + "\", \"r_station\" ],";
		}

		//remove last comma
		jsonStr = jsonStr.Remove(jsonStr.Length - 1);

		jsonStr = jsonStr + " ] ], ";
		jsonStr = jsonStr + " [ \"defineInit\", ";

		foreach (GameObject g in collectors)
		{
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"posX\", [ \"array\", \"" + g.GetComponent<Collector>().GetName() + "\" ] ], \"" + g.GetComponent<Collector>().GetPosX() + "\" ], ";
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"posY\", [ \"array\", \"" + g.GetComponent<Collector>().GetName() + "\" ] ], \"" + g.GetComponent<Collector>().GetPosY() + "\" ], ";
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"battery-amount\", [ \"array\", \"" + g.GetComponent<Collector>().GetName() + "\" ] ], \"" + g.GetComponent<Collector>().GetBatteryAmount() + "\" ], ";
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"wood-amount\", [ \"array\", \"" + g.GetComponent<Collector>().GetName() + "\" ] ], \"" + g.GetComponent<Collector>().GetWoodAmount() + "\" ], ";
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"stone-amount\", [ \"array\", \"" + g.GetComponent<Collector>().GetName() + "\" ] ], \"" + g.GetComponent<Collector>().GetStoneAmount() + "\" ], ";
			jsonStr = jsonStr + " [\"true\", [\"predicate\", \"free\", [ \"array\", \"" + g.GetComponent<Collector>().GetName() + "\" ] ] ], ";
		}

		foreach (GameObject g in producers)
		{
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"posX\", [ \"array\", \"" + g.GetComponent<Producer>().GetName() + "\" ] ], \"" + g.GetComponent<Producer>().GetPosX() + "\" ], ";
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"posY\", [ \"array\", \"" + g.GetComponent<Producer>().GetName() + "\" ] ], \"" + g.GetComponent<Producer>().GetPosY() + "\" ], ";
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"battery-amount\", [ \"array\", \"" + g.GetComponent<Producer>().GetName() + "\" ] ], \"" + g.GetComponent<Producer>().GetBatteryAmount() + "\" ], ";
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"wood-amount\", [ \"array\", \"" + g.GetComponent<Producer>().GetName() + "\" ] ], \"" + g.GetComponent<Producer>().GetWoodAmount() + "\" ], ";
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"stone-amount\", [ \"array\", \"" + g.GetComponent<Producer>().GetName() + "\" ] ], \"" + g.GetComponent<Producer>().GetStoneAmount() + "\" ], ";
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"chest-amount\", [ \"array\", \"" + g.GetComponent<Producer>().GetName() + "\" ] ], \"" + g.GetComponent<Producer>().GetChestAmount() + "\" ], ";
			jsonStr = jsonStr + " [\"true\", [\"predicate\", \"free\", [ \"array\", \"" + g.GetComponent<Producer>().GetName() + "\" ] ] ], ";
		}

		foreach (GameObject g in woods)
		{
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"posX\", [ \"array\", \"" + g.GetComponent<Wood>().GetName() + "\" ] ], \"" + g.GetComponent<Wood>().GetPosX() + "\" ], ";
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"posY\", [ \"array\", \"" + g.GetComponent<Wood>().GetName() + "\" ] ], \"" + g.GetComponent<Wood>().GetPosY() + "\" ], ";
		}

		foreach (GameObject g in stones)
		{
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"posX\", [ \"array\", \"" + g.GetComponent<Stone>().GetName() + "\" ] ], \"" + g.GetComponent<Stone>().GetPosX() + "\" ], ";
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"posY\", [ \"array\", \"" + g.GetComponent<Stone>().GetName() + "\" ] ], \"" + g.GetComponent<Stone>().GetPosY() + "\" ], ";
		}

		foreach (GameObject g in storages)
		{
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"posX\", [ \"array\", \"" + g.GetComponent<Storage>().GetName() + "\" ] ], \"" + g.GetComponent<Storage>().GetPosX() + "\" ], ";
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"posY\", [ \"array\", \"" + g.GetComponent<Storage>().GetName() + "\" ] ], \"" + g.GetComponent<Storage>().GetPosY() + "\" ], ";
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"wood-stored\", [ \"array\", \"" + g.GetComponent<Storage>().GetName() + "\" ] ], \"" + g.GetComponent<Storage>().GetWoodStored() + "\" ], ";
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"stone-stored\", [ \"array\", \"" + g.GetComponent<Storage>().GetName() + "\" ] ], \"" + g.GetComponent<Storage>().GetStoneStored() + "\" ], ";
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"chest-stored\", [ \"array\", \"" + g.GetComponent<Storage>().GetName() + "\" ] ], \"" + g.GetComponent<Storage>().GetChestStored() + "\" ], ";
		}

		foreach (GameObject g in rechargeStations)
		{
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"posX\", [ \"array\", \"" + g.GetComponent<RechargeStation>().GetName() + "\" ] ], \"" + g.GetComponent<RechargeStation>().GetPosX() + "\" ], ";
			jsonStr = jsonStr + " [\"equal\", [ \"function\", \"posY\", [ \"array\", \"" + g.GetComponent<RechargeStation>().GetName() + "\" ] ], \"" + g.GetComponent<RechargeStation>().GetPosY() + "\" ], ";
		}

		jsonStr = jsonStr.Remove(jsonStr.Length - 1);

		//define constants
		int counter = 0;
		foreach (KeyValuePair<string, int> entry in constants)
		{
			jsonStr = jsonStr + " [ \"equal\", [\"constant\", \"" + entry.Key + "\"], \"" + entry.Value + "\" ]";
			if (counter != constants.Count - 1)
			{
				jsonStr = jsonStr + ", ";
			}
			counter++;
		}
		jsonStr = jsonStr + " ], ";

		//goal still hardcoded
		jsonStr = jsonStr + goal;

		jsonStr = jsonStr + " ] }";

		Debug.Log(jsonStr);
		JObject jobject = JObject.Parse(jsonStr);

		// write JSON directly to a file
		using (StreamWriter file = File.CreateText("./Assets/JSON/Problem.json"))
		using (JsonTextWriter writer = new JsonTextWriter(file))
		{
			writer.Formatting = Formatting.Indented;
			jobject.WriteTo(writer);
		}

	}

	public static void PlanningFailed()
	{
		Debug.Log("PLANNER FAILED");
	}


}
