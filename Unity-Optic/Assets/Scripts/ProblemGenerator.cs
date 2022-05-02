using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class ProblemGenerator : MonoBehaviour
{
	public SceneChanger sceneChanger;

	public GameObject canvas;
	public List<GameObject> errorLogs = new List<GameObject>();
	public GameObject problemName;

	public GameObject container;
	public GameObject constantsContainer;

	public Sprite collectorSprite;
	public Sprite producerSprite;
	public Sprite rechargeStationSprite;

	public GameObject mainPanel;
	public GameObject addingPanel;
	public GameObject specificsPanel;
	public GameObject constantsPanel;
	public GameObject levelPanel;

	public GameObject levelSelectionButton;
	public GameObject constantsSelectionButton;
	public GameObject entitiesSelectionButton;

	public GameObject generatingText;

	public GameObject[] fieldTexts = new GameObject[7]; 
	public GameObject[] fieldInputs = new GameObject[7];

	public GameObject levelText;

	public Image objectSprite;
	public Sprite woodSprite;
	public Sprite stoneSprite;
	public Sprite storageSprite;

	[SerializeField]
	private Domain activeDomain;

	private List<GameObject> inputConstants = new List<GameObject>();

	private struct Entity
	{
		public string name;
		public string type;
		public Dictionary<string, int> beliefs;
		public Sprite mySprite;

		public Entity(string name, string type, Dictionary<string, int> beliefs, Sprite mySprite)
		{
			this.name = name;
			this.type = type;
			this.beliefs = new Dictionary<string, int>(beliefs);
			this.mySprite = mySprite;
		}

	}

	private struct Level
	{
		public int id;
		public int initialCoins;
		public List<Entity> entities;
		public Dictionary<string, Dictionary<string, int>> goals;
		public int minGridSize;

		public Level(int id)
		{
			this.id = id;
			this.entities = new List<Entity>();
			this.goals = new Dictionary<string, Dictionary<string, int>>();
			this.minGridSize = 0;
			this.initialCoins = 0;
		}

		public string ToString()
		{
			string result = "";

			result = result + "Level - " + this.id + "\n";
			foreach (Entity e in this.entities)
			{
				result = result + e.name + " - " + e.type + ": \n";
				foreach (KeyValuePair<string, int> b in e.beliefs)
				{
					result = result + " ( " + b.Key + " -> " + b.Value + " ) \n";
				}
			}
			foreach (KeyValuePair<string, Dictionary<string, int>> entry in this.goals)
			{
				result = result + "Goals for " + entry.Key + ": \n";
				foreach (KeyValuePair<string, int> kvp in entry.Value)
				{
					result = result + " ( " + kvp.Key + " -> " + kvp.Value + " ) \n";
				}
			}
			result = result + "Min Grid-Size: " + this.minGridSize + "\n";
			result = result + "Initial coins: " + this.initialCoins;
			return result;
		}
	}

	private List<Entity> problemEntities = new List<Entity>();
	private List<Entity> levelEntities = new List<Entity>();
	private Dictionary<string, int> constants = new Dictionary<string, int>();
	private Level level_1;
	private Level level_2;
	private Level level_3;
	private Level selectedLevel;
	private Level chosenLevel;

	private int coins = 0;
	public GameObject coinContainer;

    // Start is called before the first frame update
    void Start()
    {
		Application.targetFrameRate = 60;

		InstantiateLevels();

		activeDomain = Parser.ParseDomain();
		chosenLevel = new Level(0);
		GoToMainPanel();
    }

    // Update is called once per frame
    void Update()
    {
        
    }

	public void GoToMainPanel()
	{
		addingPanel.SetActive(false);
		specificsPanel.SetActive(false);
		constantsPanel.SetActive(false);
		mainPanel.SetActive(true);
		levelPanel.SetActive(false);
		coinContainer.SetActive(false);

		if (chosenLevel.id == 0)
		{
			constantsSelectionButton.GetComponent<Button>().interactable = false;
			Color originalColor = constantsSelectionButton.GetComponent<Image>().color;
			originalColor.a = 0.6f;
			constantsSelectionButton.GetComponent<Image>().color = originalColor;
		} else
		{
			constantsSelectionButton.GetComponent<Button>().interactable = true;
			Color originalColor = constantsSelectionButton.GetComponent<Image>().color;
			originalColor.a = 1f;
			constantsSelectionButton.GetComponent<Image>().color = originalColor;
		}

		if (constants.Count == 0)
		{
			entitiesSelectionButton.GetComponent<Button>().interactable = false;
			Color originalColor = entitiesSelectionButton.GetComponent<Image>().color;
			originalColor.a = 0.6f;
			entitiesSelectionButton.GetComponent<Image>().color = originalColor;
		} else
		{
			entitiesSelectionButton.GetComponent<Button>().interactable = true;
			Color originalColor = entitiesSelectionButton.GetComponent<Image>().color;
			originalColor.a = 1f;
			entitiesSelectionButton.GetComponent<Image>().color = originalColor;
		}
	}

	public void GoToLevelSelection()
	{
		addingPanel.SetActive(false);
		specificsPanel.SetActive(false);
		constantsPanel.SetActive(false);
		mainPanel.SetActive(false);
		levelPanel.SetActive(true);
		coinContainer.SetActive(false);

		levelText.GetComponent<TMP_Text>().text = "";

		selectedLevel = new Level(0);
		chosenLevel = new Level(0);

		constants = new Dictionary<string, int>();
		problemEntities = new List<Entity>();

		foreach (Transform child in constantsContainer.transform)
		{
			GameObject.Destroy(child.gameObject);
		}

		foreach (Transform child in container.transform)
		{
			GameObject.Destroy(child.gameObject);
		}
	}

	public void ChangeView(string obj)
	{
		addingPanel.SetActive(false);
		specificsPanel.SetActive(true);
		constantsPanel.SetActive(false);
		mainPanel.SetActive(false);
		levelPanel.SetActive(false);
		coinContainer.SetActive(true);

		generatingText.GetComponent<TMP_Text>().text = obj;
		ActivateAllInputFields();
		UpdateCoins();

		for (int i = 0; i < fieldInputs.Length - 1; i++)
		{
			fieldInputs[i].GetComponent<TMP_InputField>().text = "";
		}
		
		switch (obj)
		{
			case "Collector":
				fieldTexts[0].GetComponent<TMP_Text>().text = "Name: ";
				fieldTexts[1].GetComponent<TMP_Text>().text = "Battery-Amount: ";
				fieldTexts[2].GetComponent<TMP_Text>().text = "Position (X): ";
				fieldTexts[3].GetComponent<TMP_Text>().text = "Position (Y): ";
				fieldTexts[4].GetComponent<TMP_Text>().text = "Initial Wood Amount: ";
				fieldTexts[5].GetComponent<TMP_Text>().text = "Initial Stone Amount: ";
				fieldTexts[6].GetComponent<TMP_Text>().text = "";

				fieldInputs[6].SetActive(false);
				objectSprite.sprite = collectorSprite;
				break;
			case "Producer":
				fieldTexts[0].GetComponent<TMP_Text>().text = "Name: ";
				fieldTexts[1].GetComponent<TMP_Text>().text = "Battery-Amount: ";
				fieldTexts[2].GetComponent<TMP_Text>().text = "Position (X): ";
				fieldTexts[3].GetComponent<TMP_Text>().text = "Position (Y): ";
				fieldTexts[4].GetComponent<TMP_Text>().text = "Initial Wood Amount: ";
				fieldTexts[5].GetComponent<TMP_Text>().text = "Initial Stone Amount: ";
				fieldTexts[6].GetComponent<TMP_Text>().text = "Initial Chest Amount: ";

				objectSprite.sprite = producerSprite;
				break;
			case "R_Station":
				fieldTexts[0].GetComponent<TMP_Text>().text = "Name: ";
				fieldTexts[1].GetComponent<TMP_Text>().text = "";
				fieldTexts[2].GetComponent<TMP_Text>().text = "Position (X): ";
				fieldTexts[3].GetComponent<TMP_Text>().text = "Position (Y): ";
				fieldTexts[4].GetComponent<TMP_Text>().text = "";
				fieldTexts[5].GetComponent<TMP_Text>().text = "";
				fieldTexts[6].GetComponent<TMP_Text>().text = "";

				fieldInputs[1].SetActive(false);
				fieldInputs[4].SetActive(false);
				fieldInputs[5].SetActive(false);
				fieldInputs[6].SetActive(false);
				objectSprite.sprite = rechargeStationSprite;
				break;
			default:
				break;
		}
	}

	public void GoToMenu()
	{
		addingPanel.SetActive(true);
		specificsPanel.SetActive(false);
		constantsPanel.SetActive(false);
		mainPanel.SetActive(false);
		levelPanel.SetActive(false);
		coinContainer.SetActive(true);

		GameObject referenceAddedObject = (GameObject)Instantiate(Resources.Load("AddedObject"));
		UpdateCoins();

		foreach (Transform child in container.transform)
		{
			GameObject.Destroy(child.gameObject);
		}

		foreach (Entity e in levelEntities)
		{
			GameObject newObj = (GameObject)Instantiate(referenceAddedObject);
			newObj.transform.parent = container.transform;
			newObj.transform.GetChild(0).GetComponent<Image>().sprite = e.mySprite;
			newObj.GetComponentInChildren<TMP_Text>().text = e.name;
		}

		foreach (Entity e in problemEntities)
		{
			GameObject newObj = (GameObject)Instantiate(referenceAddedObject);
			newObj.transform.parent = container.transform;
			newObj.transform.GetChild(0).GetComponent<Image>().sprite = e.mySprite;
			newObj.GetComponentInChildren<TMP_Text>().text = e.name;
		}

		Destroy(referenceAddedObject);
	}

	public void GoToConstants()
	{
		addingPanel.SetActive(false);
		specificsPanel.SetActive(false);
		constantsPanel.SetActive(true);
		mainPanel.SetActive(false);
		levelPanel.SetActive(false);
		coinContainer.SetActive(true);

		this.coins = chosenLevel.initialCoins;

		UpdateCoins();

		foreach (Transform child in constantsContainer.transform)
		{
			GameObject.Destroy(child.gameObject);
		}

		problemEntities = new List<Entity>();

		constants = new Dictionary<string, int>();

		GameObject referenceAddedObject = (GameObject)Instantiate(Resources.Load("ConstantInput"));
		foreach (Belief b in activeDomain.beliefs)
		{
			if(b.type == Belief.BeliefType.Constant)
			{
				GameObject newObj = (GameObject)Instantiate(referenceAddedObject);
				newObj.transform.parent = constantsContainer.transform;
				newObj.transform.GetChild(0).GetComponent<TMP_InputField>().text = "";
				newObj.transform.GetChild(1).GetComponent<TMP_Text>().text = b.name.ToUpper();
				inputConstants.Add(newObj);
			}		
		}
		Destroy(referenceAddedObject);
	}

	public void AddEntity()
	{
		Dictionary<string, int> tempBeliefs = new Dictionary<string, int>();

		int batteryChosen = 0;
		int woodAmount = 0;
		int stoneAmount = 0;
		int chestAmount = 0;

		//"i" starts at 1 in order to avoid name
		for (int i = 1; i < fieldInputs.Length - 1; i++)
		{
			if (fieldInputs[i].active)
			{
				int value;
				bool success = int.TryParse(fieldInputs[i].GetComponent<TMP_InputField>().text, out value);

				if (success)
				{
					switch (fieldTexts[i].GetComponent<TMP_Text>().text)
					{
						case "Battery-Amount: ":
							batteryChosen = value;
							if(value > 0 && value < constants["battery-capacity"])
							{
								tempBeliefs.Add("battery-amount", value);
							} else
							{
								BadInput("Battery Amount value out of bound");
								return;
							}	
							break;
						case "Position (X): ":
							if(value >= 0 && value < constants["grid-size"])
							{
								tempBeliefs.Add("posX", value);
							} else
							{
								BadInput("PosX value out of bound");
								return;
							}			
							break;
						case "Position (Y): ":
							if (value >= 0 && value < constants["grid-size"])
							{
								tempBeliefs.Add("posY", value);
							}
							else
							{
								BadInput("PosY value out of bound");
								return;
							}
							break;
						case "Initial Wood Amount: ":
							woodAmount = value;
							if (value >= 0 && value < constants["sample-capacity"])
							{
								tempBeliefs.Add("wood-amount", value);
							}
							else
							{
								BadInput("Wood Amount value out of bound");
								return;
							}
							break;
						case "Initial Stone Amount: ":
							stoneAmount = value;
							if (value >= 0 && value < constants["sample-capacity"])
							{
								tempBeliefs.Add("stone-amount", value);
							}
							else
							{
								BadInput("Stone Amount value out of bound");
								return;
							}
							break;
						case "Initial Chest Amount: ":
							chestAmount = value;
							if (value >= 0 && value < constants["sample-capacity"])
							{
								tempBeliefs.Add("chest-amount", value);
							}
							else
							{
								BadInput("Chest Amount value out of bound");
								return;
							}
							break;
						default:
							break;
					}
				} else
				{
					BadInput("Bad value in " + fieldTexts[i].GetComponent<TMP_Text>().text);
				}
			}
		}

		//check name
		foreach (Entity e in problemEntities)
		{
			if(e.name == fieldInputs[0].GetComponent<TMP_InputField>().text)
			{
				BadInput("Name already in use: " + e.name);
				return;
			}
		}

		int cost = 0;
		cost = 120 + ((batteryChosen - 50) * 3) + woodAmount*100 + stoneAmount*100 + chestAmount*250;

		if (cost <= this.coins)
		{
			Entity toAdd = new Entity(fieldInputs[0].GetComponent<TMP_InputField>().text, generatingText.GetComponent<TMP_Text>().text.ToLower(), tempBeliefs, objectSprite.sprite);
			problemEntities.Add(toAdd);

			this.coins = this.coins - cost;
			UpdateCoins();
		} else
		{
			BadInput("Not enough coins to perform the action");
		}

		
		GoToMenu();
	}

	public void SetConstants()
	{
		bool goOn = true;

		foreach (GameObject obj in inputConstants)
		{
			int cost = 0;

			if(obj != null)
			{
				int value;
				bool success = int.TryParse(obj.transform.GetChild(0).GetComponent<TMP_InputField>().text, out value);

				if (success)
				{
					if(obj.transform.GetChild(1).GetComponent<TMP_Text>().text == "BATTERY-CAPACITY")
					{
						cost = (value - 100) * 5;
					}

					if (cost <= this.coins)
					{
						if (!(obj.transform.GetChild(1).GetComponent<TMP_Text>().text == "GRID-SIZE") || value >= chosenLevel.minGridSize)
						{
							constants.Add(obj.transform.GetChild(1).GetComponent<TMP_Text>().text.ToLower(), value);
							this.coins = this.coins - cost;
						}
						else
						{
							BadInput("Grid size lower than minimum requested by the level");
							goOn = false;
						}
					} else
					{
						goOn = false;
						BadInput("Not enough coins to perform the action");
					}

				}
				else
				{
					BadInput("Bad value in " + obj.transform.GetChild(1).GetComponent<TMP_Text>().text);
					goOn = false;
				}
			}

		}

		


		if (goOn)
		{
			GoToMainPanel();
		} else
		{
			constants = new Dictionary<string, int>();
		}
			
	}

	public void SelectLevel(int levelId)
	{


		switch (levelId)
		{
			case 1:
				selectedLevel = level_1;
				break;
			case 2:
				selectedLevel = level_2;
				break;
			case 3:
				selectedLevel = level_3;
				break;
			default:
				break;
		}

		levelText.GetComponent<TMP_Text>().text = selectedLevel.ToString();
		
	}

	public void ConfirmLevel()
	{
		chosenLevel = selectedLevel;
		levelEntities = new List<Entity>();

		foreach (Entity e in chosenLevel.entities)
		{
			levelEntities.Add(e);
		}
		this.coins = chosenLevel.initialCoins;
		GoToMainPanel();
	}

	public void GenerateProblem()
	{

		string pName = problemName.GetComponent<TMP_InputField>().text;

		if(pName != "")
		{
			string jsonStr = "";
			jsonStr = jsonStr + "{ \"problem\" : [ \"defineProblem\", \"" + pName + "\", \"" + activeDomain.name + "\", ";
			jsonStr = jsonStr + " [\"defineObjects\", [\"array\", ";

			int counter = 0;
			
			foreach (Entity e in levelEntities)
			{
				jsonStr = jsonStr + " [ \"parameter\", \"" + e.name + "\", \"" + e.type + "\" ],";
			}
			
			foreach (Entity e in problemEntities)
			{
				jsonStr = jsonStr + " [ \"parameter\", \"" + e.name + "\", \"" + e.type + "\" ]";
				if(counter != problemEntities.Count - 1)
				{
					jsonStr = jsonStr + ", ";
				}

				counter++;
			}
			jsonStr = jsonStr + " ] ], ";
			jsonStr = jsonStr + " [ \"defineInit\", ";

			//define functions and predicates
			foreach (Entity e in problemEntities)
			{
				//functions
				foreach (KeyValuePair<string, int> entry in e.beliefs)
				{
					jsonStr = jsonStr + " [\"equal\", [ \"function\", \"" + entry.Key + "\", [ \"array\", \"" + e.name + "\" ] ], \"" + entry.Value + "\" ], ";
				}
				//predicates
				if(e.type == "collector" || e.type == "producer")
				{
					jsonStr = jsonStr + " [\"true\", [\"predicate\", \"free\", [ \"array\", \"" + e.name + "\" ] ] ], ";
				}
			}
			foreach (Entity e in levelEntities)
			{
				//functions
				foreach (KeyValuePair<string, int> entry in e.beliefs)
				{
					jsonStr = jsonStr + " [\"equal\", [ \"function\", \"" + entry.Key + "\", [ \"array\", \"" + e.name + "\" ] ], \"" + entry.Value + "\" ], ";
				}
				//predicates
				if (e.type == "collector" || e.type == "producer")
				{
					jsonStr = jsonStr + " [\"true\", [\"predicate\", \"free\", [ \"array\", \"" + e.name + "\" ] ] ], ";
				}
			}

			//define constants
			counter = 0;
			foreach (KeyValuePair<string, int> entry in constants)
			{
				jsonStr = jsonStr + " [ \"equal\", [\"constant\", \"" + entry.Key + "\"], \"" + entry.Value + "\" ]";
				if(counter != constants.Count - 1)
				{
					jsonStr = jsonStr + ", ";
				}
				counter++;
			}
			jsonStr = jsonStr + " ], ";

			//goal still hardcoded
			jsonStr = jsonStr + " [\"defineGoal\", ";

			counter = 0;
			int secondCounter = 0;
			foreach (KeyValuePair<string, Dictionary<string, int>> entry in chosenLevel.goals)
			{
				foreach (KeyValuePair<string, int> kvp in entry.Value)
				{
					jsonStr = jsonStr + " [\"equal\", [\"function\", \"" + kvp.Key + "\", [\"array\", \"" + entry.Key + "\" ] ], \"" + kvp.Value + "\" ]";

					if (counter != chosenLevel.goals.Count - 1 || secondCounter != entry.Value.Count - 1)
					{
						jsonStr = jsonStr + ", ";
					}
					secondCounter++;
				}
				counter++;
			}

			jsonStr = jsonStr + " ] ] }";

			JObject jobject = JObject.Parse(jsonStr);

			// write JSON directly to a file
			using (StreamWriter file = File.CreateText("./Assets/JSON/Problem.json"))
			using (JsonTextWriter writer = new JsonTextWriter(file))
			{
				writer.Formatting = Formatting.Indented;
				jobject.WriteTo(writer);
			}

		} else
		{
			BadInput("Missing Problem Name");
			return;
		}

		sceneChanger.MoveToMainMenu();
	}

	private void ActivateAllInputFields()
	{
		for (int i = 0; i < fieldInputs.Length; i++)
		{
			fieldInputs[i].SetActive(true);
		}
	}

	private void InstantiateLevels()
	{
		level_1 = new Level(1);
		List<Entity> lvl_1_entities = new List<Entity>();
		lvl_1_entities.Add(CreateResource("w1", "wood", 3, 3, woodSprite));
		lvl_1_entities.Add(CreateResource("w2", "wood", 9, 9, woodSprite));
		lvl_1_entities.Add(CreateResource("s1", "stone", 3, 9, stoneSprite));
		lvl_1_entities.Add(CreateResource("s2", "stone", 9, 3, stoneSprite));
		lvl_1_entities.Add(CreateStorage("st", 6, 6, storageSprite));
		Dictionary<string, Dictionary<string, int>> lvl_1_goals = new Dictionary<string, Dictionary<string, int>>();
		Dictionary<string, int> lvl_1_subgoals = new Dictionary<string, int>();
		lvl_1_subgoals.Add("wood-stored", 3);
		lvl_1_subgoals.Add("stone-stored", 8);
		lvl_1_goals.Add("st", lvl_1_subgoals);
		level_1.entities = lvl_1_entities;
		level_1.goals = lvl_1_goals;
		level_1.minGridSize = 10;
		level_1.initialCoins = 500;


		level_2 = new Level(2);
		List<Entity> lvl_2_entities = new List<Entity>();
		lvl_2_entities.Add(CreateResource("w1", "wood", 9, 3, woodSprite));
		lvl_2_entities.Add(CreateResource("w2", "wood", 9, 9, woodSprite));
		lvl_2_entities.Add(CreateResource("s1", "stone", 3, 3, stoneSprite));
		lvl_2_entities.Add(CreateResource("s2", "stone", 3, 9, stoneSprite));
		lvl_2_entities.Add(CreateStorage("st1", 3, 6, storageSprite));
		lvl_2_entities.Add(CreateStorage("st2", 9, 6, storageSprite));
		Dictionary<string, Dictionary<string, int>> lvl_2_goals = new Dictionary<string, Dictionary<string, int>>();
		Dictionary<string, int> lvl_2_subgoals_st1 = new Dictionary<string, int>();
		lvl_2_subgoals_st1.Add("wood-stored", 5);
		lvl_2_goals.Add("st1", lvl_2_subgoals_st1);
		Dictionary<string, int> lvl_2_subgoals_st2 = new Dictionary<string, int>();
		lvl_2_subgoals_st2.Add("stone-stored", 5);
		lvl_2_goals.Add("st2", lvl_2_subgoals_st2);
		level_2.entities = lvl_2_entities;
		level_2.goals = lvl_2_goals;
		level_2.minGridSize = 10;
		level_2.initialCoins = 500;


		level_3 = new Level(3);
		List<Entity> lvl_3_entities = new List<Entity>();
		lvl_3_entities.Add(CreateResource("w1", "wood", 3, 3, woodSprite));
		lvl_3_entities.Add(CreateResource("w2", "wood", 9, 9, woodSprite));
		lvl_3_entities.Add(CreateResource("s1", "stone", 3, 9, stoneSprite));
		lvl_3_entities.Add(CreateResource("s2", "stone", 9, 3, stoneSprite));
		lvl_3_entities.Add(CreateStorage("st", 6, 6, storageSprite));
		Dictionary<string, Dictionary<string, int>> lvl_3_goals = new Dictionary<string, Dictionary<string, int>>();
		Dictionary<string, int> lvl_3_subgoals = new Dictionary<string, int>();
		lvl_3_subgoals.Add("chest-stored", 10);
		lvl_3_goals.Add("st", lvl_3_subgoals);
		level_3.entities = lvl_3_entities;
		level_3.goals = lvl_3_goals;
		level_3.minGridSize = 10;
		level_3.initialCoins = 750;

	}

	private Entity CreateResource(string name, string type, int posX, int posY, Sprite sprite)
	{
		Dictionary<string, int> tempBeliefs = new Dictionary<string, int>();
		tempBeliefs.Add("posX", posX);
		tempBeliefs.Add("posY", posY);
		return new Entity(name, type, tempBeliefs, sprite);
	}

	private Entity CreateStorage(string name, int posX, int posY, Sprite sprite)
	{
		Dictionary<string, int> tempBeliefs = new Dictionary<string, int>();
		tempBeliefs.Add("posX", posX);
		tempBeliefs.Add("posY", posY);
		tempBeliefs.Add("wood-stored", 0);
		tempBeliefs.Add("stone-stored", 0);
		tempBeliefs.Add("chest-stored", 0);
		return new Entity(name, "storage", tempBeliefs, sprite);
	}

	private void BadInput(string why)
	{
		GameObject referenceErrorLog = (GameObject)Instantiate(Resources.Load("ErrorMessage"), canvas.transform);
		referenceErrorLog.transform.GetChild(0).GetComponent<TMP_Text>().text = "ERROR -- " + why;
		referenceErrorLog.GetComponent<ErrorLog>().StartFade();

		errorLogs.Add(referenceErrorLog);
		foreach (GameObject e in errorLogs)
		{
			if(e != null)
				e.GetComponent<ErrorLog>().StartMove();
		}

		//Debug.Log("ERROR -- " + why);
		return;
	}

	private void UpdateCoins()
	{
		coinContainer.transform.GetChild(1).GetComponent<TMP_Text>().text = this.coins.ToString();
	}
}
