using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class ErrorLog : MonoBehaviour
{

    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        
    }

	public void StartFade()
	{
		StartCoroutine(Fade());
	}

	public IEnumerator Fade()
	{
		float transparency = 1f;
		
		for (int i = 0; i < 255; i++)
		{
			Color newColor = gameObject.GetComponent<Image>().color;
			newColor.a = transparency;
			gameObject.GetComponent<Image>().color = newColor;

			Color newColorText = gameObject.transform.GetChild(0).GetComponent<TMP_Text>().color;
			newColorText.a = transparency;
			gameObject.transform.GetChild(0).GetComponent<TMP_Text>().color = newColorText;

			Color newColorIcon = gameObject.transform.GetChild(1).GetComponent<Image>().color;
			newColorIcon.a = transparency;
			gameObject.transform.GetChild(1).GetComponent<Image>().color = newColorIcon;

			transparency -= 1f / 255f;

			yield return null;
		}
		DestroyObject(gameObject);
	}

	public void StartMove()
	{
		StartCoroutine(Move());
	}

	public IEnumerator Move()
	{
		for (int i = 0; i < 50; i++)
		{
			gameObject.transform.position = new Vector2(gameObject.transform.position.x, gameObject.transform.position.y - 2f);
			yield return null;
		}
	}
}
