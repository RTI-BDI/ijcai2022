using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class UIManager : MonoBehaviour
{
	public Camera mainCamera;
	public GameObject background;

	public GameObject setCanvas;
	public static GameObject canvas;
	public static List<GameObject> errorLogs = new List<GameObject>();

	[SerializeField]
	private GameObject setPanel;
	private static GameObject panel;

	[SerializeField]
	private GameObject setText_1;
	private static GameObject text_1;

	[SerializeField]
	private GameObject setText_2;
	private static GameObject text_2;

	[SerializeField]
	private GameObject setText_3;
	private static GameObject text_3;

	[SerializeField]
	private GameObject setText_4;
	private static GameObject text_4;

	[SerializeField]
	private GameObject setText_5;
	private static GameObject text_5;

	[SerializeField]
	private GameObject setText_6;
	private static GameObject text_6;

	[SerializeField]
	private GameObject setText_7;
	private static GameObject text_7;

	[SerializeField]
	private GameObject setText_8;
	private static GameObject text_8;

	[SerializeField]
	private Image setSprite;
	private static Image sprite;

	[SerializeField]
	private GameObject setSlider;
	private static GameObject slider;

	[SerializeField]
	private GameObject setTextSlider;
	private static GameObject textSlider;

	public static string inspectedObj = "";

	[SerializeField]
	private GameObject setPlayButton;
	private static GameObject playButton;

	[SerializeField]
	private GameObject setPauseButton;
	private static GameObject pauseButton;

	[SerializeField]
	private GameObject setPausePanel;
	private static GameObject pausePanel;

	[SerializeField]
	private GameObject setFrameText;
	private static GameObject frameText;

	[SerializeField]
	private GameObject setCoinText;
	private static GameObject coinText;

	void Start()
	{
		canvas = setCanvas;

		panel = setPanel;
		text_1 = setText_1;
		text_2 = setText_2;
		text_3 = setText_3;
		text_4 = setText_4;
		text_5 = setText_5;
		text_6 = setText_6;
		text_7 = setText_7;
		text_8 = setText_8;
		sprite = setSprite;
		slider = setSlider;
		textSlider = setTextSlider;
		coinText = setCoinText;

		playButton = setPlayButton;
		pauseButton = setPauseButton;
		pausePanel = setPausePanel;
		frameText = setFrameText;

		panel.SetActive(false);
		pausePanel.SetActive(false);
	}

	void LateUpdate()
	{
		background.transform.position = new Vector2(mainCamera.transform.position.x, mainCamera.transform.position.y);
	}

	public static void SetVisibleCollector(string objName, int objPosX, int objPosY, int objBatteryAmount, int objWoodAmount, int objStoneAmount, Sprite objSprite)
	{
		inspectedObj = objName;

		//Activate the panel
		if(panel != null)
		{
			panel.SetActive(true);
		}

		if(text_1 != null)
		{
			text_1.SetActive(true);
			text_1.transform.GetChild(0).GetComponent<TMP_Text>().text = "Name: " + objName;
		}

		if(text_2 != null)
		{
			text_2.SetActive(true);
			text_2.transform.GetChild(0).GetComponent<TMP_Text>().text = "X-Position: " + objPosX;
		}

		if(text_3 != null)
		{
			text_3.SetActive(true);
			text_3.transform.GetChild(0).GetComponent<TMP_Text>().text = "Y-Position: " + objPosY;
		}

		if(text_4 != null)
		{
			text_4.SetActive(true);
			text_4.transform.GetChild(0).GetComponent<TMP_Text>().text = "Battery Level: " + objBatteryAmount + "%";
		}

		if (text_5 != null)
		{
			text_5.SetActive(true);
			text_5.transform.GetChild(0).GetComponent<TMP_Text>().text = "Wood Carried: " + objWoodAmount;
		}

		if (text_6 != null)
		{
			text_6.SetActive(true);
			text_6.transform.GetChild(0).GetComponent<TMP_Text>().text = "Stone Carried: " + objStoneAmount;
		}

		if (text_7 != null)
		{
			text_7.SetActive(false);
			text_7.transform.GetChild(0).GetComponent<TMP_Text>().text = "";
		}

		if (text_8 != null)
		{
			text_8.SetActive(false);
			text_8.transform.GetChild(0).GetComponent<TMP_Text>().text = "";
		}

		if (sprite != null)
		{
			sprite.sprite = objSprite;
		}

		if(slider != null)
		{
			slider.SetActive(true);
			slider.GetComponent<Slider>().maxValue = GameManager.GetConstants()["battery-capacity"];
			slider.GetComponent<Slider>().value = objBatteryAmount;
		}

		if(textSlider != null)
		{
			textSlider.GetComponent<TMP_Text>().text = objBatteryAmount.ToString();
		}
	}

	public static void UpdateCollectorPanel(string objName, int objPosX, int objPosY, int objBatteryAmount, int objWoodAmount, int objStoneAmount, Sprite objSprite)
	{
		if(panel.active && inspectedObj == objName)
		{
			SetVisibleCollector(objName, objPosX, objPosY, objBatteryAmount, objWoodAmount, objStoneAmount, objSprite);
		}
	}

	public static void SetVisibleProducer(string objName, int objPosX, int objPosY, int objBatteryAmount, int objWoodAmount, int objStoneAmount, int objChestAmount, Sprite objSprite)
	{
		inspectedObj = objName;

		//Activate the panel
		if (panel != null)
		{
			panel.SetActive(true);
		}

		if (text_1 != null)
		{
			text_1.SetActive(true);
			text_1.transform.GetChild(0).GetComponent<TMP_Text>().text = "Name: " + objName;
		}

		if (text_2 != null)
		{
			text_2.SetActive(true);
			text_2.transform.GetChild(0).GetComponent<TMP_Text>().text = "X-Position: " + objPosX;
		}

		if (text_3 != null)
		{
			text_3.SetActive(true);
			text_3.transform.GetChild(0).GetComponent<TMP_Text>().text = "Y-Position: " + objPosY;
		}

		if (text_4 != null)
		{
			text_4.SetActive(true);
			text_4.transform.GetChild(0).GetComponent<TMP_Text>().text = "Wood Carried: " + objWoodAmount;
		}

		if (text_5 != null)
		{
			text_5.SetActive(true);
			text_5.transform.GetChild(0).GetComponent<TMP_Text>().text = "Stone Carried: " + objStoneAmount;
		}

		if (text_6 != null)
		{
			text_6.SetActive(true);
			text_6.transform.GetChild(0).GetComponent<TMP_Text>().text = "Chests Carried: " + objChestAmount;
		}

		if (text_7 != null)
		{
			text_7.SetActive(false);
			text_7.transform.GetChild(0).GetComponent<TMP_Text>().text = "";
		}

		if (text_8 != null)
		{
			text_8.SetActive(false);
			text_8.transform.GetChild(0).GetComponent<TMP_Text>().text = "";
		}

		if (sprite != null)
		{
			sprite.sprite = objSprite;
		}

		if (slider != null)
		{
			slider.SetActive(true);
			slider.GetComponent<Slider>().maxValue = GameManager.GetConstants()["battery-capacity"];
			slider.GetComponent<Slider>().value = objBatteryAmount;
		}

		if (textSlider != null)
		{
			textSlider.GetComponent<TMP_Text>().text = objBatteryAmount.ToString();
		}
	}

	public static void UpdateProducerPanel(string objName, int objPosX, int objPosY, int objBatteryAmount, int objWoodAmount, int objStoneAmount, int objChestAmount, Sprite objSprite)
	{
		if (panel.active && inspectedObj == objName)
		{
			SetVisibleProducer(objName, objPosX, objPosY, objBatteryAmount, objWoodAmount, objStoneAmount, objChestAmount, objSprite);
		}
	}

	public static void SetVisibleWood(string objName, int objPosX, int objPosY, Sprite objSprite)
	{
		inspectedObj = objName;

		//Activate the panel
		if (panel != null)
		{
			panel.SetActive(true);
		}

		if (text_1 != null)
		{
			text_1.SetActive(true);
			text_1.transform.GetChild(0).GetComponent<TMP_Text>().text = "Name: " + objName;
		}

		if (text_2 != null)
		{
			text_2.SetActive(true);
			text_2.transform.GetChild(0).GetComponent<TMP_Text>().text = "X-Position: " + objPosX;
		}

		if (text_3 != null)
		{
			text_3.SetActive(true);
			text_3.transform.GetChild(0).GetComponent<TMP_Text>().text = "Y-Position: " + objPosY;
		}

		if (text_4 != null)
		{
			text_4.SetActive(true);
			text_4.transform.GetChild(0).GetComponent<TMP_Text>().text = "Resource: Wood";
		}

		if (text_5 != null)
		{
			text_5.SetActive(false);
			text_5.transform.GetChild(0).GetComponent<TMP_Text>().text = "";
		}

		if (text_6 != null)
		{
			text_6.SetActive(false);
			text_6.transform.GetChild(0).GetComponent<TMP_Text>().text = "";
		}

		if (text_7 != null)
		{
			text_7.SetActive(false);
			text_7.transform.GetChild(0).GetComponent<TMP_Text>().text = "";
		}

		if (text_8 != null)
		{
			text_8.SetActive(false);
			text_8.transform.GetChild(0).GetComponent<TMP_Text>().text = "";
		}

		if (sprite != null)
		{
			sprite.sprite = objSprite;
		}

		if (slider != null)
		{
			slider.SetActive(false);
		}
	}

	public static void SetVisibleStone(string objName, int objPosX, int objPosY, Sprite objSprite)
	{
		inspectedObj = objName;

		//Activate the panel
		if (panel != null)
		{
			panel.SetActive(true);
		}

		if (text_1 != null)
		{
			text_1.SetActive(true);
			text_1.transform.GetChild(0).GetComponent<TMP_Text>().text = "Name: " + objName;
		}

		if (text_2 != null)
		{
			text_2.SetActive(true);
			text_2.transform.GetChild(0).GetComponent<TMP_Text>().text = "X-Position: " + objPosX;
		}

		if (text_3 != null)
		{
			text_3.SetActive(true);
			text_3.transform.GetChild(0).GetComponent<TMP_Text>().text = "Y-Position: " + objPosY;
		}

		if (text_4 != null)
		{
			text_4.SetActive(true);
			text_4.transform.GetChild(0).GetComponent<TMP_Text>().text = "Resource: Stone";
		}

		if (text_5 != null)
		{
			text_5.SetActive(false);
			text_5.transform.GetChild(0).GetComponent<TMP_Text>().text = "";
		}

		if (text_6 != null)
		{
			text_6.SetActive(false);
			text_6.transform.GetChild(0).GetComponent<TMP_Text>().text = "";
		}

		if (text_7 != null)
		{
			text_7.SetActive(false);
			text_7.transform.GetChild(0).GetComponent<TMP_Text>().text = "";
		}

		if (text_8 != null)
		{
			text_8.SetActive(false);
			text_8.transform.GetChild(0).GetComponent<TMP_Text>().text = "";
		}

		if (sprite != null)
		{
			sprite.sprite = objSprite;
		}

		if (slider != null)
		{
			slider.SetActive(false);
		}
	}

	public static void SetVisibleRechargeStation(string objName, int objPosX, int objPosY, Sprite objSprite)
	{
		inspectedObj = objName;

		//Activate the panel
		if (panel != null)
		{
			panel.SetActive(true);
		}

		if (text_1 != null)
		{
			text_1.SetActive(true);
			text_1.transform.GetChild(0).GetComponent<TMP_Text>().text = "Name: " + objName;
		}

		if (text_2 != null)
		{
			text_2.SetActive(true);
			text_2.transform.GetChild(0).GetComponent<TMP_Text>().text = "X-Position: " + objPosX;
		}

		if (text_3 != null)
		{
			text_3.SetActive(true);
			text_3.transform.GetChild(0).GetComponent<TMP_Text>().text = "Y-Position: " + objPosY;
		}

		if (text_4 != null)
		{
			text_4.SetActive(true);
			text_4.transform.GetChild(0).GetComponent<TMP_Text>().text = "Entity: Recharge station";
		}

		if (text_5 != null)
		{
			text_5.SetActive(false);
			text_5.transform.GetChild(0).GetComponent<TMP_Text>().text = "";
		}

		if (text_6 != null)
		{
			text_6.SetActive(false);
			text_6.transform.GetChild(0).GetComponent<TMP_Text>().text = "";
		}

		if (text_7 != null)
		{
			text_7.SetActive(false);
			text_7.transform.GetChild(0).GetComponent<TMP_Text>().text = "";
		}

		if (text_8 != null)
		{
			text_8.SetActive(false);
			text_8.transform.GetChild(0).GetComponent<TMP_Text>().text = "";
		}

		if (sprite != null)
		{
			sprite.sprite = objSprite;
		}

		if (slider != null)
		{
			slider.SetActive(false);
		}
	}

	public static void SetVisibleStorage(string objName, int objPosX, int objPosY, int objWoodStored, int objStoneStored, int objChestStored, Sprite objSprite)
	{
		inspectedObj = objName;

		//Activate the panel
		if (panel != null)
		{
			panel.SetActive(true);
		}

		if (text_1 != null)
		{
			text_1.SetActive(true);
			text_1.transform.GetChild(0).GetComponent<TMP_Text>().text = "Name: " + objName;
		}

		if (text_2 != null)
		{
			text_2.SetActive(true);
			text_2.transform.GetChild(0).GetComponent<TMP_Text>().text = "X-Position: " + objPosX;
		}

		if (text_3 != null)
		{
			text_3.SetActive(true);
			text_3.transform.GetChild(0).GetComponent<TMP_Text>().text = "Y-Position: " + objPosY;
		}

		if (text_4 != null)
		{
			text_4.SetActive(true);
			text_4.transform.GetChild(0).GetComponent<TMP_Text>().text = "Wood Stored: " + objWoodStored;
		}

		if (text_5 != null)
		{
			text_5.SetActive(true);
			text_5.transform.GetChild(0).GetComponent<TMP_Text>().text = "Stone Stored: " + objStoneStored;
		}

		if (text_6 != null)
		{
			text_6.SetActive(true);
			text_6.transform.GetChild(0).GetComponent<TMP_Text>().text = "Chests Stored: " + objStoneStored;
		}

		if (text_7 != null)
		{
			text_7.SetActive(true);
			text_7.transform.GetChild(0).GetComponent<TMP_Text>().text = "Entity: Storage";
		}

		if (text_8 != null)
		{
			text_8.SetActive(false);
			text_8.transform.GetChild(0).GetComponent<TMP_Text>().text = "";
		}

		if (sprite != null)
		{
			sprite.sprite = objSprite;
		}

		if(slider != null)
		{
			slider.SetActive(false);
		}
	}

	public static void UpdateStoragePanel(string objName, int objPosX, int objPosY, int objWoodStored, int objStoneStored, int objChestStored, Sprite objSprite)
	{
		if (panel.active && inspectedObj == objName)
		{
			SetVisibleStorage(objName, objPosX, objPosY, objWoodStored, objStoneStored, objChestStored, objSprite);
		}
	}

	public static void ClosePanel()
	{
		if(panel != null)
		{
			panel.SetActive(false);
		}
	}

	public void OnSliderChange(float value)
	{
		textSlider.GetComponent<TMP_Text>().text = ((int)value).ToString();
	}

	public static void UpdateBattery(KeyValuePair<GameObject, string> obj, int coins)
	{
		int oldBattery = 0;

		if(obj.Value == "collector")
		{
			oldBattery = obj.Key.GetComponent<Collector>().GetBatteryAmount();
		} else
		{
			oldBattery = obj.Key.GetComponent<Producer>().GetBatteryAmount();
		}

		if(coins > (5*(Mathf.Abs(oldBattery - int.Parse(textSlider.GetComponent<TMP_Text>().text)))))
		{
			Dictionary<string, int> update = new Dictionary<string, int>();
			update.Add("battery-amount_" + inspectedObj, int.Parse(textSlider.GetComponent<TMP_Text>().text));
			Parser.UpdateSensors(update, "SET", GameManager.GetFrame());

			GameManager.DescreaseCoins((5 * (Mathf.Abs(oldBattery - int.Parse(textSlider.GetComponent<TMP_Text>().text)))));

			if (obj.Value == "collector")
			{
				obj.Key.GetComponent<Collector>().SetBatteryAmount(int.Parse(textSlider.GetComponent<TMP_Text>().text));
			}
			else
			{
				obj.Key.GetComponent<Producer>().SetBatteryAmount(int.Parse(textSlider.GetComponent<TMP_Text>().text));
			}

		} else
		{
			CoinError("Not enought coins to change the battery amount.");
		}
		

		
	}

	public static void InPause()
	{
		Color originalColor = playButton.GetComponent<Image>().color;
		originalColor.a = 1f;
		playButton.GetComponent<Image>().color = originalColor;
		playButton.GetComponent<Button>().enabled = true;

		originalColor = pauseButton.GetComponent<Image>().color;
		originalColor.a = 0.6f;
		pauseButton.GetComponent<Image>().color = originalColor;
		pauseButton.GetComponent<Button>().enabled = false;

		pausePanel.SetActive(true);

	}

	public static void InPlay()
	{
		Color originalColor = playButton.GetComponent<Image>().color;
		originalColor.a = 0.6f;
		playButton.GetComponent<Image>().color = originalColor;
		playButton.GetComponent<Button>().enabled = false;

		originalColor = pauseButton.GetComponent<Image>().color;
		originalColor.a = 1f;
		pauseButton.GetComponent<Image>().color = originalColor;
		pauseButton.GetComponent<Button>().enabled = true;

		pausePanel.SetActive(false);
	}

	public static void UpdateFrameText(int frame)
	{
		frameText.GetComponent<TMP_Text>().text = frame.ToString();
	}

	public static void UpdateCoinText(int coins)
	{
		coinText.GetComponent<TMP_Text>().text = coins.ToString();
	}

	public static void CoinError(string why)
	{
		GameObject referenceErrorLog = (GameObject)Instantiate(Resources.Load("ErrorMessage"), canvas.transform);
		referenceErrorLog.transform.GetChild(0).GetComponent<TMP_Text>().text = "ERROR -- " + why;
		referenceErrorLog.GetComponent<ErrorLog>().StartFade();
		referenceErrorLog.GetComponent<RectTransform>().localScale = new Vector3(0.5f, 0.5f, 1);

		errorLogs.Add(referenceErrorLog);
		foreach (GameObject e in errorLogs)
		{
			if (e != null)
				e.GetComponent<ErrorLog>().StartMove();
		}

		//Debug.Log("ERROR -- " + why);
		return;
	}

}
