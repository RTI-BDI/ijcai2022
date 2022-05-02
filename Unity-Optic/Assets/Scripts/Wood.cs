using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Wood : MonoBehaviour
{
    [SerializeField]
    private string name;
    [SerializeField]
    private int posX;
    [SerializeField]
    private int posY;

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
				UIManager.SetVisibleWood(this.name, this.posX, this.posY, gameObject.GetComponent<SpriteRenderer>().sprite);
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
            default:
                break;
        }
    }

    public void MoveToDestination(float tileSize, Vector2 position)
    {
        gameObject.transform.position = new Vector2(position.x + (posX * tileSize), position.y + (posY * tileSize));
    }

	public void CollectWood() { }

	public int GetPosX()
	{
		return this.posX;
	}

	public int GetPosY()
	{
		return this.posY;
	}
}
