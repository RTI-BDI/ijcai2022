using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Storage : MonoBehaviour
{

    [SerializeField]
    private string name;
    [SerializeField]
    private int posX;
    [SerializeField]
    private int posY;
    [SerializeField]
    private int woodStored;
    [SerializeField]
    private int stoneStored;
    [SerializeField]
    private int chestStored;

	private bool isPaused = false;

    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
		//Inspect Object
		if (Input.GetMouseButtonDown(1))
		{
			Collider2D hitCollider = Physics2D.OverlapPoint(Camera.main.ScreenToWorldPoint(Input.mousePosition));
			if (hitCollider != null && hitCollider.transform == gameObject.transform)
			{
				UIManager.SetVisibleStorage(this.name, this.posX, this.posY, this.woodStored, this.stoneStored, this.chestStored, gameObject.GetComponent<SpriteRenderer>().sprite);
			}
		}
	}

    public void SetName(string name)
    {
        this.name = name;
    }

    public string GetName()
    {
        return this.name;
    }

    public void SetAttribute(string attribute, string value)
    {
        switch (attribute)
        {

            case "posX":
                this.posX = int.Parse(value);
                break;
            case "posY":
                this.posY = int.Parse(value);
                break;
            case "wood-stored":
                this.woodStored = int.Parse(value);
                break;
            case "stone-stored":
                this.stoneStored = int.Parse(value);
                break;
            case "chest-stored":
                this.chestStored = int.Parse(value);
                break;
            default:
                break;
        }
    }

    public void MoveToDestination(float tileSize, Vector2 position)
    {
        gameObject.transform.position = new Vector2(position.x + (posX * tileSize) - (tileSize / 2), position.y + (posY * tileSize) + (tileSize / 2));
    }

	public int GetPosX()
	{
		return this.posX;
	}

	public int GetPosY()
	{
		return this.posY;
	}

	public int GetWoodStored()
	{
		return this.woodStored;
	}

	public int GetStoneStored()
	{
		return this.stoneStored;
	}

	public int GetChestStored()
	{
		return this.chestStored;
	}

	public IEnumerator StoreWood()
    {
        int actionTime = 40;

        for (int i = 0; i < actionTime; i++)
        {
			while (isPaused)
			{
				yield return null;
			}

			yield return null;
        }

        //Actual effect
        this.woodStored++;

		Dictionary<string, int> toUpdate = new Dictionary<string, int>();
		toUpdate.Add("wood-stored_" + this.name, this.woodStored);
		Parser.UpdateSensors(toUpdate, "SET", GameManager.GetFrame());
	}

    public IEnumerator StoreStone()
    {
        int actionTime = 40;

        for (int i = 0; i < actionTime; i++)
        {
			while (isPaused)
			{
				yield return null;
			}

			yield return null;
        }

        //Actual effect
        this.stoneStored++;

		Dictionary<string, int> toUpdate = new Dictionary<string, int>();
		toUpdate.Add("stone-stored_" + this.name, this.stoneStored);
		Parser.UpdateSensors(toUpdate, "SET", GameManager.GetFrame());
	}

    public IEnumerator StoreChest()
    {
        int actionTime = 40;

        for (int i = 0; i < actionTime; i++)
        {
			while (isPaused)
			{
				yield return null;
			}

			yield return null;
        }

        //Actual effect
        this.chestStored++;

		Dictionary<string, int> toUpdate = new Dictionary<string, int>();
		toUpdate.Add("chest-stored_" + this.name, this.chestStored);
		Parser.UpdateSensors(toUpdate, "SET", GameManager.GetFrame());
	}

	private void UpdatePanel()
	{
		UIManager.UpdateStoragePanel(this.name, this.posX, this.posY, this.woodStored, this.stoneStored, this.chestStored, gameObject.GetComponent<SpriteRenderer>().sprite);
	}

	public void Pause()
	{
		isPaused = true;
	}

	public void Resume()
	{
		isPaused = false;
	}

	public void StopAction(string action)
	{
		StopCoroutine(action);
	}

	public void StopAllActions()
	{
		StopAllCoroutines();
	}

}
